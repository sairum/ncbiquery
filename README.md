# ncbiquery
Fetch DNA sequences from NCBI

**ncbiquery** is an attempt to develop a Qt6 c++ class to deal with queries to the [Genbank database](https://www.ncbi.nlm.nih.gov/genbank/about/). GenBank is the most up to date and comprehensive DNA sequence information database.

## Querying GenBank

Queries to GenBank are made through a REST API using HTTP requests through specific endpoints, namely *esearch* and *efetch*, hosted in the National Center of Biotechnology Information (NCBI) facilities.  The repository maybe queried in many different ways. The goal of **ncbiquery** is to retrieve DNA sequences of specific *organisms* (using their scientific names) for specific genes or DNA markers. Hence, a query term will always consist of two parameters:

* a scientific name (using any taxa available at [NCBI's taxonomy database](https://www.ncbi.nlm.nih.gov/taxonomy)) followed by the tag '[organism]' (without quotes)

* a gene or marker name followed by the tag '[gene]' (without quotes). Several genes/markers may be combined in the same query separated by the word 'OR' (without quotes)

A typical query term would be:

```
Corophium volutator[organism] AND COI[gene]
```

which returns **four** records of sequences of *cytochrome oxidase c subunit I* (COI) with roughly 650 base pairs each (in 28/10/2024). Some genes/markers may be known by more than one name or acronym. COI is one of them. It is also known as *CO1* or *COX1*! Hence, the above query term may be formulated as

```
Corophium volutator[organism] AND (COI[gene] OR COX1[gene] OR CO1[gene])
```

in which case **50** records are returned!

## A Qt6 Class to query GenBank

The Qt class developed to query and process GenBank data will be named *GbQuery* and should retrieve all GenBank records given a well formed query term and get rid of any superfluous information. At first this task may seem trivial. However, a typical GenBank request is made of at least two distinct HTTPS calls through NCBI's REST API. The first request uses the endpoint *esearch* and returns all record IDs in GenBank that match the query term. These IDs are called *GenBank IDs* or *GIs* (or *GIDs*) and are unique within GenBank. An example of a search for sequences of COI for *Corophium volutator* (an amphipod) would be:

```
https://eutils.ncbi.nlm.nih.gov/entrez/eutils/esearch.fcgi?db=nuccore&term=Corophium+volutator[organism]+AND+COI[gene]&retmax=20
```

Note that spaces are not allowed in the HTTP request, so they were replaced by a '+' character. The result of such request is a XML document which contains, among other information, the number of records found (in the \<Count\> tagt) plus the GIs of the matching records enclosed in \<Id\> tags:

```
<eSearchResult>
  <Count>4</Count>
  <RetMax>4</RetMax>
  <RetStart>0</RetStart>
  <IdList>
    <Id>936254122</Id>
    <Id>936253636</Id>
    <Id>936253006</Id>
    <Id>936252832</Id>
    </IdList>
  <TranslationSet>
  ...
</eSearchResult>
```

With this list (936254122, 936253636, 936253006, and 936252832), a second call to NCBI's REST API using the *efetch* endpoint is done to retrieve the records themselves. The new query should be:

```
https://eutils.ncbi.nlm.nih.gov/entrez/eutils/efetch.fcgi?db=nuccore&id=936254122,936253636,936253006,936252832&retmax=20&rettype=gb&retmode=xml
```

The result is returned in a XML stream, which looks like the list below. A \<GBSet\> is made up of several \<GBSeq\> records (each corresponding to a unique *GI* retrieved in the first call) with extensive information about the sequence retrieved, such as the *accession number* (KT209362), the *date of record creation* (10-OCT-2015), etc. The last element of a \<GBSeq\> is a DNA sequence itself (\<GBSeq_sequence\>).

```
<GBSet>
  <GBSeq>
    <GBSeq_locus>KT209362</GBSeq_locus>
    <GBSeq_length>658</GBSeq_length>
    <GBSeq_strandedness>double</GBSeq_strandedness>
    <GBSeq_moltype>DNA</GBSeq_moltype>
    <GBSeq_topology>linear</GBSeq_topology>
    <GBSeq_division>INV</GBSeq_division>
    <GBSeq_update-date>10-OCT-2015</GBSeq_update-date>
    <GBSeq_create-date>10-OCT-2015</GBSeq_create-date>
    <GBSeq_definition>
        Corophium volutator voucher MT02430 cytochrome oxidase subunit 1 (COI) gene, partial cds; mitochondrial
    </GBSeq_definition>
    <GBSeq_primary-accession>KT209362</GBSeq_primary-accession>
    <GBSeq_accession-version>KT209362.1</GBSeq_accession-version>
    <GBSeq_other-seqids>
      <GBSeqid>gnl|uoguelph|BNSA067-12.COI-5P</GBSeqid>
      <GBSeqid>gb|KT209362.1|</GBSeqid>
      <GBSeqid>gi|936254122</GBSeqid>
    </GBSeq_other-seqids>
    ...
    <GBSeq_sequence>
        aactctttattttatcttaggaacttggtccggattagtagggacctctataagaataattattcgaactgaattaagagggcccgg
        aaatttaattggtaatgaccaaatttataacgtaattgtgactgcacacgcttttattataatttttttcatagttataccgatcat
        aattgggggatttgggaattgacttgtacctttaatattaggtagacctgatatagctttcccacgaataaacaatataagattttg
        attactgcctccttcattaacattccttgttatatctagtttactagaaagaggtgtaggaaccgggtgaacagtttaccctccctt
        gagctcatccattgctcatagaggtggtgctgttgatttagctatcttttctcttcatttagcgggggcttcttctattctaggggc
        tattaattttatttcaacagtaatcaacatacgatcagtaggaatatatataaaccgagtcccacttttcgtttggtctgtctttat
        tactgccatcctacttttattatccctcccagtattagccggagctattactatattacttacagaccggaatattaatacatcatt
        ttttgatccattgggaggaggagaccctattctttaccaacatttattt
    </GBSeq_sequence>
  </GBSeq>
  <GBSeq>
  ...
</GBSet>
```

## The Qt6 Class *GbQuery*

Usage of a REST API implies dealing with HTTP requests. For security reasons, NCBI mandates that all requests to its API are made through HTTPS. In Qt6 this calls for an object of type [QNetworkAccessManager](https://doc.qt.io/qt-6/qnetworkaccessmanager.html), which encapsulates most of the details of HTTP(S) connections. Hence, the class *GbQuery* implements its own *QNetworkAccessManager* named *_manager*. This object is responsible for building the URLs used for searching and fetching records (using organisms and genes/markers names). It submits the request using a GET method and technically waits for the reply which fires a *finished* signal.

### Processing results from two different requests (*esearch* and *efetch*)

The whole process of finding and retrieving data from GenBank is asynchronous. Usually, the *_manager* object submits a query to the *esearch* API endpoint using its *get()* method, which returns a pointer to a  [QNetworkReply](https://doc.qt.io/qt-6/qnetworkreply.html) object, named *_reply*.


```
QNetworkReply *reply = _manager->get( _request );
```

The *_reply* object will contain the result (in XML format) or an error, and should be processed once the *QNetworkAccessManager::finished* signal is fired. This signal is usually connected with a slot of *_manager* which is able to process the resulting XML. In the present implementation this slot would be *_manager->processESearch()*. Note the *would be*. Things are more complicated as you can see next.

If the previous XML result has been successfully processed, the *_manager* object should now have a list of *GIs* to fetch from GenBank. First, it should delete the former *QNetworkReply*. A new request should be made using the *_manager->get()* method, but this time the request is adapted to use the URL for the *efetch* API endpoint of GenBank. However, the slot connected to the *QNetworkAccessManager::finished* signal is still the method to process the result from *esearch* API endpoint. Both results are sent in XML but they are completely different. The slot *_manager->processESearch()* cannot process the new XML result.

At this stage there are three options: 1) we destroy the old connection and create a new one pointing to a specific method (slot *_manager->processEFetch()*) or 2) we develop a single method that is able to parse both XML results from *esearch* and *efetch*, or 3) instead of using the *QNetworkAccessManager::finished* signal, we use the *QNetworkReply::finished* signal and connect each to its respective XML processing slot: *_manager->processESearch()* and *_manager->processEFetch()*.

Right now the *GbQuery* Class uses the latter option. However, there is an additional problem. Contrary to *QNetworkAccessManager*, the *finished* signal of *QNetworkReply* does not carry the object a pointer the the reply that triggered it. This makes it necessary to resort to the *sender()* method in order to obtain a pointer to the reply object.

```
QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender() );
```

Finally, to complicate things further, NCBI searches should be limited to a 'reasonable' amount of returned records. After all, it is a public and free service and people should use it sparingly. The default is usually to return 20 records per search and this can be explicitly set in the query to *esearch* through the *retmax* option. However, a normal search may return a result set that includes more than *retmax* records. In these cases, the *esearch* tool should keep track of a variable that stores where to begin retrieving records (or, put in another way, how many records to skip before starting retrieving records). This variable called *retstart* (a
is returned in each query to *esearch* or *efetch*. The *esearch* result formatted as XML provides among other things (e.g. the list of *GIs*), the total number of records matched by the query string in a variable named \<Count\> (see above). If \<Count\> is larger than the sum of *retstart* and *retmax*, the variable *retstart* is increased by *retmax* records. The *GbQuery* object emits a new *search* SIGNAL, but this time its parameter should carry the new *retstart*.

```
                        SIGNAL GbQuery::search( retstart = 0 )
                                          |
                                          V
                      SLOT GbQuery::searchNCBI( retstart )
                             ^            |
     ________________________|            |
     |                                    V
     |                     SIGNAL QNetworkReply::finished
     |                                    |
     |                                    V
     |                     SLOT GbQuery::processESearch
     |                                    |
     |                                    V
     |                get count ( = total records matching query )
     |                get retmax records' ids starting at retstart
     |                                    |
     |                                    V
     |                    _______________________________
     |                    |                             |
     |                    |                             V
     |                    |                 GbQuery::fetchFromNCBI()
     |                    V                             |
     |        count > retstart + retmax                 V
     |          |                  |       SIGNAL QNetworkReply::finished
     |          |                  |                    |
     |         Yes                 No                   |
     |          |                  |                    V
     |          V                  |       SLOT GbQuery::processEfetch
     |  retstart += retmax         |                    |
     |          |                  |                    |
     |          v                  |                    |
SIGNAL GbQuery::search( retstart ) |                    |
                                   |                    |
                                   |                    |
                                   V                    V
                                  STOP                 STOP
```








