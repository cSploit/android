
Network Working Group                                     J. Sermersheim
Internet-Draft                                               Novell, Inc
Expires: August 26, 2005                               February 22, 2005



               Distributed Procedures for LDAP Operations
                 draft-sermersheim-ldap-distproc-02.txt


Status of this Memo


   This document is an Internet-Draft and is subject to all provisions
   of Section 3 of RFC 3667.  By submitting this Internet-Draft, each
   author represents that any applicable patent or other IPR claims of
   which he or she is aware have been or will be disclosed, and any of
   which he or she become aware will be disclosed, in accordance with
   RFC 3668.


   Internet-Drafts are working documents of the Internet Engineering
   Task Force (IETF), its areas, and its working groups.  Note that
   other groups may also distribute working documents as
   Internet-Drafts.


   Internet-Drafts are draft documents valid for a maximum of six months
   and may be updated, replaced, or obsoleted by other documents at any
   time.  It is inappropriate to use Internet-Drafts as reference
   material or to cite them other than as "work in progress."


   The list of current Internet-Drafts can be accessed at
   http://www.ietf.org/ietf/1id-abstracts.txt.


   The list of Internet-Draft Shadow Directories can be accessed at
   http://www.ietf.org/shadow.html.


   This Internet-Draft will expire on August 26, 2005.


Copyright Notice


   Copyright (C) The Internet Society (2005).


Abstract


   This document provides the data types and procedures used while
   servicing Lightweight Directory Access Protocol (LDAP) user
   operations in order to participate in a distributed directory.  In
   particular, it describes the way in which an LDAP user operation in a
   distributed directory environment finds its way to the proper DSA(s)
   for servicing.





Sermersheim              Expires August 26, 2005                [Page 1]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



Discussion Forum


   Technical discussion of this document will take place on the IETF
   LDAP Extensions mailing list <ldapext@ietf.org>.  Please send
   editorial comments directly to the author.


Table of Contents


   1.   Distributed Operations Overview  . . . . . . . . . . . . . .   3
   2.   Conventions  . . . . . . . . . . . . . . . . . . . . . . . .   4
   3.   Distributed Operation Data Types . . . . . . . . . . . . . .   5
   3.1  ContinuationReference  . . . . . . . . . . . . . . . . . . .   5
   3.2  ChainedRequest . . . . . . . . . . . . . . . . . . . . . . .   9
   3.3  Chained Response . . . . . . . . . . . . . . . . . . . . . .  11
   4.   Distributed Procedures . . . . . . . . . . . . . . . . . . .  14
   4.1  Name resolution  . . . . . . . . . . . . . . . . . . . . . .  14
   4.2  Operation Evaluation . . . . . . . . . . . . . . . . . . . .  16
   4.3  Populating the ContinuationReference . . . . . . . . . . . .  19
   4.4  Sending a ChainedRequest . . . . . . . . . . . . . . . . . .  21
   4.5  Emulating the Sending of a ChainedRequest  . . . . . . . . .  23
   4.6  Receiving a ChainedRequest . . . . . . . . . . . . . . . . .  24
   4.7  Returning a Chained Response . . . . . . . . . . . . . . . .  25
   4.8  Receiving a Chained Response . . . . . . . . . . . . . . . .  26
   4.9  Returning a Referral or Intermediate Referral  . . . . . . .  27
   4.10 Acting on a Referral or Intermediate Referral  . . . . . . .  30
   4.11 Ensuring non-existence of an entry under an nssr . . . . . .  31
   4.12 Mapping a referralURI to an LDAP URI . . . . . . . . . . . .  31
   4.13 Using the ManageDsaIT control  . . . . . . . . . . . . . . .  32
   5.   Security Considerations  . . . . . . . . . . . . . . . . . .  33
   6.   Normative References . . . . . . . . . . . . . . . . . . . .  33
        Author's Address . . . . . . . . . . . . . . . . . . . . . .  34
   A.   IANA Considerations  . . . . . . . . . . . . . . . . . . . .  35
   A.1  LDAP Object Identifier Registrations . . . . . . . . . . . .  35
   A.2  LDAP Protocol Mechanism Registrations  . . . . . . . . . . .  35
   A.3  LDAP Descriptor Registrations  . . . . . . . . . . . . . . .  37
   A.4  LDAP Result Code Registrations . . . . . . . . . . . . . . .  38
        Intellectual Property and Copyright Statements . . . . . . .  39















Sermersheim              Expires August 26, 2005                [Page 2]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



1.  Distributed Operations Overview


   One characteristic of X.500-based directory systems [X500] is that,
   given a distributed Directory Information Tree (DIT), a user should
   potentially be able to have any service request satisfied (subject to
   security, access control, and administrative policies) irrespective
   of the Directory Service Agent (DSA) to which the request was sent.
   To accommodate this requirement, it is necessary that any DSA
   involved in satisfying a particular service request have some
   knowledge (as specified in {TODO: Link to future Distributed Data
   Model doc}) of where the requested information is located and either
   return this knowledge to the requester or attempt to satisfy the
   request satisfied on the behalf of the requester (the requester may
   either be a Directory User Agent (DUA) or another DSA).


   Two modes of operation distribution are defined to meet these
   requirements, namely "chaining" and "returning referrals".
   "Chaining" refers to the attempt by a DSA to satisfy a request by
   sending one or more chained operations to other DSAs.  "Returning
   referrals", is the act of returning distributed knowledge information
   to the requester, which may then itself interact with the DSA(s)
   identified by the distributed knowledge information.  It is a goal of
   this document to provide the same level of service whether the
   chaining or referral mechanism is used to distribute an operation.


   The processing of an operation is talked about in two major phases,
   namely "name resolution", and "operation evaluation".  Name
   resolution is the act of locating a local DSE held on a DSA given a
   distinguished name (DN).  Operation evaluation is the act of
   performing the operation after the name resolution phase is complete.


   While distributing an operation, a request operation may be
   decomposed into several sub-operations.


   The distributed directory operation procedures described in this
   document assume the absense of the ManageDsaIT control defined in
   [RFC3296] and described in Section 4.13.















Sermersheim              Expires August 26, 2005                [Page 3]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



2.  Conventions


   Imperative keywords defined in [RFC2119] are used in this document,
   and carry the meanings described there.


   All Basic Encoding Rules (BER) [X690] encodings follow the
   conventions found in Section 5.1 of [RFC2251].













































Sermersheim              Expires August 26, 2005                [Page 4]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



3.  Distributed Operation Data Types


   The data types in this section are used by the chaining and referral
   distributed operation mechanisms described in Section 4


3.1  ContinuationReference


   As an operation is being processed by a DSA, it is useful to group
   the information passed between various procedures as a collection of
   data.  The ContinuationReference data type is introduced for this
   purpose.  This data type is populated and consumed by various
   procedures discussed in various sections of this document.  In
   general, a ContinuationReference is used when indicating that
   directory information being acted on is not present locally, but may
   be present elsewhere.


   A ContinuationReference consists of one or more addresses which
   identify remote DSAs along with other information pertaining both to
   the distributed knowledge information held on the local DSA as well
   as information relevant to the operation.  This data type is
   expressed here in Abstract Syntax Notation One (ASN.1) [X680].


      ContinuationReference ::= SET {
         referralURI      [0] SET SIZE (1..MAX) OF URI,
         localReference   [2] LDAPDN,
         referenceType    [3] ReferenceType,
         remainingName    [4] RelativeLDAPDN OPTIONAL,
         searchScope      [5] SearchScope OPTIONAL,
         searchedSubtrees [6] SearchedSubtrees OPTIONAL,
         failedName       [7] LDAPDN OPTIONAL,
         ...  }


   <Editor's Note: Planned for addition is a searchCriteria field which
   is used both for assuring that the remote object is in fact the
   object originally pointed to (this mechanism provides a security
   measure), and also to allow moved or renamed remote entries to be
   found.  Typically the search criteria would have a filter value of
   (entryUUID=<something>)>


   URI ::= LDAPString     -- limited to characters permitted in URIs
   [RFC2396].


      ReferenceType ::= ENUMERATED {
         superior               (0),
         subordinate            (1),
         cross                  (2),
         nonSpecificSubordinate (3),





Sermersheim              Expires August 26, 2005                [Page 5]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



         suplier                (4),
         master                 (5),
         immediateSuperior      (6),
         self                   (7),
         ...  }
      SearchScope ::= ENUMERATED {
         baseObject         (0),
         singleLevel        (1),
         wholeSubtree       (2),
         subordinateSubtree (3),
         ...  }


   SearchedSubtrees ::= SET OF RelativeLDAPDN


   LDAPDN, RelativeLDAPDN, and LDAPString, are defined in [RFC2251].


   The following subsections introduce the fields of the
   ContinuationReference data type, but do not provide in-depth
   semantics or instructions on the population and consumption of the
   fields.  These topics are discussed as part of the procedural
   instructions.


3.1.1  ContinuationReference.referralURI


   The list of referralURI values is used by the receiver to progress
   the operation.  Each value specifies (at minimum) the protocol and
   address of one or more remote DSA(s) holding the data sought after.
   URI values which are placed in ContinuationReference.referralURI must
   allow for certain elements of data to be conveyed.  Section 3.1.1.1
   describes these data elements.  Furthermore, a mapping must exist
   which relates the parts of a specified URI to these data elements.
   This document provides such a mapping for the LDAP URL [RFC2255] in
   Section 4.12.


   In some cases, a referralURI will contain data which has a
   counterpart in the fields of the ContinuationReference (an example is
   where the referralURI is an LDAP URL, holds a <scope> value, and the
   ContinuationReference.searchScope field is also present).  In these
   cases, the data held on the referralURI overrides the field in the
   ContinuationReference.  Specific examples of this are highlighted in
   other sections.  Providing a means for these values to exist as
   fields of the ContinuationReference allows one value to be applied to
   all values of referralURI (as opposed to populating duplicate data on
   all referralURI values).


   If a referralURI value identifies an LDAP-enabled DSA [RFC3377], the
   LDAP URL form is used.





Sermersheim              Expires August 26, 2005                [Page 6]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



3.1.1.1  Elements of referralURI Values


   The following data elements must be allowed and identified for a
   specified URI type to be used to convey referral information.  Each
   element is given a name which begins with 'referralURI.' for clarity
   when referencing the elements conceptually in other parts of this
   document.


   o  referralURI.protocolIdentifier.  There must be an indication of
      the protocol to be used to contact the DSA identified by the URI.
   o  referralURI.accessPoint.  The URI must identify a DSA in a manner
      that can be used to contact it using the protocol specified in
      protocolIdentifier.
   o  referralURI.targetObject.  Holds the name to be used as the base
      DN of the operation being progressed.  This field must be allowed
      by the URI specification, but may be omitted in URI instances for
      various reasons.
   o  referralURI.localReference.  See Section 3.1.2.  This field must
      be allowed by the URI specification, but may be omitted in URI
      instances for various reasons.
   o  referralURI.searchScope.  See Section 3.1.5.  This field must be
      allowed by the URI specification, but may be omitted in URI
      instances for various reasons.
   o  referralURI.searchedSubtrees.  See Section 3.1.6.  This field must
      be allowed by the URI specification, but may be omitted in URI
      instances for various reasons.
   o  referralURI.failedName.  See Section 3.1.7.  This field must be
      allowed by the URI specification, but may be omitted in URI
      instances for various reasons.


3.1.2  ContinuationReference.localReference


   This names the DSE which was found to hold distributed knowledge
   information, and thus which caused the ContinuationReference to be
   formed.  This field is primarily used to help convey the new target
   object name, but may also be used for purposes referential integrity
   (not discussed here).  In the event that the root object holds the
   distributed knowledge information, this field is present and is
   populated with an empty DN.


3.1.3  ContinuationReference.referenceType


   Indicates the DSE Type of the ContinuationReference.localReference.
   This field may be used to determine how to progress an operations
   (i.e.  if the value is nonSpecificSubordinate, a search continuation
   will exclude the ContinuationReference.referenceType).






Sermersheim              Expires August 26, 2005                [Page 7]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



3.1.4  ContinuationReference.remainingName


   In certain scenarios, the localReference does not completely name the
   DSE to be used as the new target object name.  In these cases,
   remainingName is populated with the RDNSequence relative to the
   localReference of the target object name being resolved.  Some
   examples of these scenarios include (but are not restricted to):


   o  During name resolution, the name is not fully resolved, but a DSE
      holding distributed knowledge information is found, causing a
      ContinuationReference to be generated.
   o  While searching, an alias is dereferenced.  The aliasedObjectName
      points to a DSE of type glue which is subordinate to a DSE holding
      distributed knowledge information.


3.1.5  ContinuationReference.searchScope


   Under certain circumstances, when progressing a search operation, a
   search scope different than that of the original search request must
   be used.  This field facilitates the conveyance of the proper search
   scope to be used when progressing the distributed operation.


   The scope of subordinateSubtree has been added to the values allowed
   by the LDAP SearchRequest.scope field.  This scope includes the
   subtree of entries below the base DN, but does not include the base
   DN itself.  This is used here when progressing distributed search
   operations caused by the existence of a DSE of type nssr.


   If a referralURI.searchScope is present, it overrides this field
   while that referralURI is being operated upon.


3.1.6  ContinuationReference.searchedSubtrees


   For ContinuationReferences generated while processing a search
   operation with a scope of wholeSubtree, each value of this field
   indicates that a particular subtree below the target object has
   already been searched.  Consumers of this data use it to cause the
   progression of the search operation to exclude these subtrees as a
   mechanism to avoid receiving duplicate entries.


   If a referralURI.searchedSubtrees is present, it overrides this field
   while that referralURI is being operated upon.


3.1.7  ContinuationReference.failedName


   When an operation requires that multiple names be resolved (as is the
   case with the ModifyDN operation), this field is used to specify
   which name was found to be non-local.




Sermersheim              Expires August 26, 2005                [Page 8]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



   If a referralURI.failedName is present, it overrides this field while
   that referralURI is being operated upon.


3.2  ChainedRequest


   The Chained Request is sent as an LDAP extended operation.  The
   requestName is IANA-ASSIGNED-OID.1.  The requestValue is the BER
   encoding of the following ChainedRequestValue ASN.1 definition:


      ChainedRequestValue ::= SEQUENCE {


         chainingArguments ChainingArguments,
         operationRequest  OperationRequest }


      ChainingArguments ::= SEQUENCE {


         targetObject     [0] LDAPDN OPTIONAL,
         referenceType    [1] ReferenceType,
         traceInformation [2] ChainingTraceInformation,
         searchScope      [3] SearchScope OPTIONAL,
         searchedSubtrees [4] SearchedSubtrees OPTIONAL}


      ChainingTraceInformation ::= SET OF LDAPURL


      OperationRequest ::= SEQUENCE {


         Request ::= CHOICE {


            bindRequest    BindRequest,
            searchRequest  SearchRequest,
            modifyRequest  ModifyRequest,
            addRequest     AddRequest,
            delRequest     DelRequest,
            modDNRequest   ModifyDNRequest,
            compareRequest CompareRequest,
            extendedReq    ExtendedRequest,
            ...  },
         controls       [0] Controls COPTIONAL }


   BindRequest, SearchRequest, ModifyRequest, AddRequest, DelRequest,
   ModifyDNRequest, CompareRequest, ExtendedRequest and Controls are
   defined in [RFC2251].


3.2.1  ChainedRequestValue.chainingArguments


   In general, these fields assist in refining the original operation as
   it is to be executed on the receiving DSA.





Sermersheim              Expires August 26, 2005                [Page 9]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



3.2.1.1  ChainedRequestValue.chainingArguments.targetObject


   This field contains the new target (or base) DN for the operation.


   The sending DSA populates this under different scenarios including
   the case where an alias has been dereferenced while resolving the DN,
   and also the case where a referral carries a target name different
   from the reference object that caused the referral.


   This field can be omitted only if it would be the the same value as
   the object or base object parameter in the
   ChainedRequestValue.operationRequest, in which case its implied value
   is that value.


   The receiving DSA examines this field and (if present) uses it rather
   than the base DN held in the ChainedRequestValue.operationRequest.


3.2.1.2  ChainedRequestValue.chainingArguments.referenceType


   See Section 3.1.3.


   If the receiver encounters a value of nonSpecificSubordinate in this
   field, it indicates that the operation is being chained due to DSE of
   type nssr.  In this case, the receiver allows (and expects) the base
   DN to name the immediate superior of a context prefix.


3.2.1.3  ChainedRequestValue.chainingArguments.traceInformation


   This contains a set of URIs.  Each value represents the address of a
   DSA and DN that has already been contacted while attempting to
   service the operation.  This field is used to detect looping while
   servicing a distributed operation.


   The sending DSA populates this with its own URI, and also the URIs of
   any DSAs that have already been chained to.  The receiving DSA
   examines this list of URIs and returns a loopDetect error if it finds
   that any of the addresses and DNs in the listed URI's represent it's
   own.


3.2.1.4  ChainedRequestValue.chainingArguments.searchScope


   See Section 3.1.5.


3.2.1.5  ChainedRequestValue.chainingArguments.searchedSubtrees


   See Section 3.1.6.






Sermersheim              Expires August 26, 2005               [Page 10]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



3.2.2  ChainedRequestValue.operationRequest


   This holds the original LDAP operation request.  This is restricted
   to a subset of all LDAP operations.  Namely, the following LDAP
   operation types are not allowed:


   o  Abandon/Cancel operations.  When an abandon or cancel operation
      needs to be chained, it is sent to the remote DSA as-is.  This is
      because there is no need to track it for loop detection or pass on
      any other information normally found in ChainingArguments.
   o  Unbind.  Again, there is no need to send chaining-related
      information to a DSA to perform an unbind.  DSAs which chain
      operations maintain connections as they see fit.
   o  Chained Operation.  When a DSA receives a chained operation, and
      must again chain that operation to a remote DSA, it sends a
      ChainedRequest where the ChainedRequestValue.operationRequest is
      that of the incoming ChainedRequestValue.operationRequest.


3.3  Chained Response


   The Chained Response is sent as an LDAP IntermediateResponse
   [RFC3771], or LDAP ExtendedResponse [RFC2251], depending on whether
   the operation is complete or not.  In either case, the responseName
   is omitted.  For intermediate responses, the
   IntermediateResponse.responseValue is the BER encoding of the
   ChainedIntermediateResponseValue ASN.1 definition.  For completed
   operations, the ExtendedResponse.value is the BER encoding of the
   ChainedFinalResponseValue ASN.1 definition.


      ChainedIntermediateResponseValue ::= SEQUENCE {


         chainedResults    ChainingResults,
         operationResponse IntermediateResponse }


      ChainedFinalResponseValue ::= SEQUENCE {


         chainedResults    ChainingResults,
         operationResponse FinalResponse }


      ChainingResults ::= SEQUENCE {


         searchedSubtrees [0] SearchedSubtrees OPTIONAL,
         ...  }


      IntermediateResponse ::= SEQUENCE {


         Response ::= CHOICE {





Sermersheim              Expires August 26, 2005               [Page 11]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005




            searchResEntry       SearchResultEntry,
            searchResRef         SearchResultReference,
            intermediateResponse IntermediateResponse
            ...  },
         controls [0] Controls COPTIONAL }


      FinalResponse ::= SEQUENCE {


         Response ::= CHOICE {


            bindResponse    BindResponse,
            searchResDone   SearchResultDone,
            modifyResponse  ModifyResponse,
            addResponse     AddResponse,
            delResponse     DelResponse,
            modDNResponse   ModifyDNResponse,
            compareResponse CompareResponse,
            extendedResp    ExtendedResponse,
            ...  },
         controls [0] Controls COPTIONAL }


   BindResponse, SearchResultEntry, SearchResultDone,
   SearchResultReference, ModifyResponse, AddResponse, DelResponse,
   ModifyDNResponse, CompareResponse, ExtendedResponse, and Controls are
   defined in [RFC2251].  IntermediateResponse is defined in [RFC3771].


3.3.1  ChainingResults


   In general, this is used to convey additional information that may
   needed in the event that the operation needs to be progressed
   further.


3.3.1.1  ChainingResults.searchedSubtrees


   Each value of this field indicates that a particular subtree below
   the target object has already been searched.  This is particularly
   useful while chaining search operations during operation evaluation
   caused by the presence of a DSA of type nssr.  Each DSA referenced by
   the nssr holds one or more naming contexts subordinate to the nssr
   DSE.  The ChainingResults.searchedSubtrees field allows the DSA being
   chained to, to inform the sending DSA which subordinate naming
   contexts have been searched.  This information may be passed to
   further DSAs listed on the nssr in order to reduce the possibility of
   duplicate entries being returned.







Sermersheim              Expires August 26, 2005               [Page 12]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



3.3.2  ChainedIntermediateResponseValue.intermediateResponse and
      ChainedFinalResponseValue.finalResponse


   This holds the directory operation response message tied to the
   ChainedRequestValue.operationRequest.















































Sermersheim              Expires August 26, 2005               [Page 13]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



4.  Distributed Procedures


   For the purposes of describing a distributed operation, operations
   are said to consist of two major phases -- name resolution and
   operation evaluation.  These terms are adopted from [X518].  Name
   resolution is the act of locating a DSE said to be held locally by a
   DSA given a distinguished name (DN).  Operation evaluation is the act
   of performing the operation after the name resolution phase is
   complete.


   Furthermore, there are two modes of distributing an operation --
   chaining, and returning referrals.  Chaining is the act of forwarding
   an unfinished operation to another DSA for completion (this may
   happen during name resolution or operation evaluation).  In this
   case, the forwarding DSA sends a chained operation to a receiving
   DSA, which attempts to complete the operation.  Alternately, the DSA
   may return a referral (or intermediate referral), and the client may
   use that referral in order to forward the unfinished operation to
   another DSA.  Whether the operation is distributed via chaining or
   referrals is a decision left to the DSA and or DUA.


   The term 'intermediate referral' describes a referral returned during
   the operation evaluation phase of an operation.  These include
   searchResultReferences, referrals returned with an
   intermediateResponse [RFC3771], or future referrals which indicate
   that they are intermediate referrals.


   An operation which is distributed while in the operation evaluation
   phase is termed a 'sub-operation'.


   This document inserts a step between the two distributed operation
   phases in order to commonize the data and processes followed prior to
   chaining an operation or returning a referral.  This step consists of
   populating a ContinuationReference data type.


4.1  Name resolution


   Before evaluating (enacting) most directory operations, the DSE named
   by the target (often called the base DN) of the operation must be
   located .  This is done by evaluating the RDNs of the target DN one
   at a time, starting at the rootmost RDN.  Each RDN is compared to the
   DSEs held by the DSA until the set of RDNs is exhausted, or an RDN
   cannot be found.


   If the DSE named by the target is found to be local, the name
   resolution phase of the operation completes and the operation
   evaluation phase begins.





Sermersheim              Expires August 26, 2005               [Page 14]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



   If it is found that the target does not name a local DSE nor a DSE
   that may held by another DSA, it is said that the target does not
   exist, and the operation fails with noSuchObject (subject to local
   policy).


   If it is found that the DSE named by the target is non-local to the
   DSA, but may reside elsewhere, name resolution is said to be
   incomplete.  In this case, the operation may be distributed by
   creating a ContinuationReference (Section 4.3) and either chaining
   the operation (Section 4.4 and Section 4.5)or returning a referral
   (Section 4.9).


4.1.1  Determining that a named DSE is local to a DSA


   If a DSE held by a DSA falls within a naming context held by the DSA,
   or is the root DSE on a first-level DSA, it is said to be local to
   that DSA


4.1.2  Determining that a named DSE does not exist


   A named DSE is said to not exist if, during name resolution the DSE
   is not found, but if found it would fall within a naming context held
   by the DSA.


4.1.3  Determining that a named DSE is non-local


   If a named DSE is niether found to be local to the DSA, nor found to
   not exist, it is said to be non-local to a DSA.  In this case, it is
   indeterminate whether the named DSE exists.


   When a named DSE is found to be non-local, there should be
   distributed knowledge information available to be used to either
   return a referral or chain the operation.


4.1.3.1  Locating distributed knowledge information for a non-local
        target


   If it has been determined that a target names a non-local DSE,
   distributed knowledge information may be found by first examining the
   DSE named by the target, and subsequently all superior DSEs beginning
   with the immediate superior and ending with the root, until an
   examined DSE is one of types:


   {TODO: should DSE types be all caps? It would be easier to read.}
   o  subr
   o  supr
   o  immsupr





Sermersheim              Expires August 26, 2005               [Page 15]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



   o  xr
   o  nssr


   The examined DSE which is of one of these types holds the distributed
   knowledge information for the non-local named target.  This DSE is
   said to be the found distributed knowledge information of the
   non-local target.  This found distributed knowledge information may
   then be used to distribute the operation.


   If no examined DSEs are of any of these types, the distributed
   knowledge information is mis-configured, and the error
   invalidReference is returned.


4.1.4  Special case for the Add operation


   During the name resolution phase of the Add operation, the immediate
   parent of the base DN is resolved.


   If the immediate parent of the entry to be added is a DSE of type
   nssr, then further interrogation is needed to ensure that the entry
   to be added does not exist.  Methods for doing this are found in
   Section 4.11.  {TODO: don't make this mandatory.  Also, it doesn't
   work without transaction semantics.  Same prob in the mod dn below.}.


4.1.5  Special case for the ModifyDN operation


   When the modifyDN operation includes a newSuperior name, it must be
   resolved as well as the base DN being modified.  If either of these
   result in a non-local name, the name causing the operation to be
   distributed should be conveyed (Section 4.3.5).  {TODO: also mention
   access control problems, and mention (impl detail) that
   affectsmultidsa can be used.}


   If during operation evaluation of a ModifyDN operation, the
   newSuperior names a DSE type of nssr, then further interrogation is
   needed to ensure that the entry to be added does not exist.  Methods
   for doing this are found in Section 4.11.


4.2  Operation Evaluation


   Once name resolution has completed.  The DSE named in the target has
   been found to be local to a DSA.  At this point the operation can be
   carried out.  During operation evaluation distributed knowledge
   information may be found that may cause the DSA to distribute the
   operation.  When this happens, the operation may be distributed by
   creating a ContinuationReference (Section 4.3) and either chaining
   the operation (Section 4.4 and Section 4.5)or returning a referral
   (Section 4.9).




Sermersheim              Expires August 26, 2005               [Page 16]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



   If, during the location of the distributed knowledge information, the
   distributed knowledge information is found to be mis-configured,
   operation semantics are followed (some operations may call for an
   error to be returned, while others call for the error to be ignored).
   {TODO: either make this more specific, or less specific, or just toss
   it out.}


4.2.1  Search operation


   During operation evaluation of a search operation, the DSA must
   determine whether there is distributed knowledge information in the
   scope of the search.  Any DSE in the search scope which is of the
   following types is considered to be 'found distributed knowledge
   information' {TODO: use a better term than found distributed
   knowledge information} in the search scope:


   o  subr
   o  nssr (see nssr note)
   o  xr {TODO: I think xr only qualifies when an alias is dereferenced
      to an xr.  Otherwisw, there should always be a subr above the xr
      if it falls in the search scope.}


   Note that due to alias dereferencing, the search scope may expand to
   include entries outside of the scope originally specified in the
   search operation.  {TODO: note that an aliased object may be glue
   which needs to result in any subr or xr above it to be found}


   Nssr Note: A DSE of type nssr is only considered to be found
   distributed knowledge information when the scope of the search
   includes entries below it.  For example, when the search scope is
   wholeSubtree or subordinateSubtree and a DSE of type nssr is found in
   the scope, or if the search scope is singleLevel and the target
   object names a DSE of type nsssr.


   {TODO: The following sections are talking about how the continuation
   reference is to be populated.  Move to next secion.  Can probably
   just say that whole subtree or subordinare subtree encountering nssr,
   and single level rooted at nssr result in a continuation reference.
   base at, and single level above do not result in a continuation
   reference.}


4.2.1.1  Search operation with singleLevel scope


   If distributed knowledge information is found during operation
   evaluation of a search with a singleLevel scope, it will cause the
   resulting ContinuationReference.searchScope to be set to baseObject.






Sermersheim              Expires August 26, 2005               [Page 17]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



4.2.1.2  Search operation encountering nssr knowledge reference


   When a search operation encounters distributed knowledge information
   which is a DSE type of nssr during operation evaluation, the
   following instructions are followed:


   Note that when a search operation is being progressed due to nssr
   knowledge information, the subsequent distributed progression of the
   search is caused to be applied to each DSA listed as non-specific
   knowledge information (This is talked about in Section 4.3.2).  In
   the event that multiple DSAs listed in the knowledge information hold
   copies of the same directory entries, the 'already searched' and
   'duplicate elimination' mechanisms SHOULD be used to prevent
   duplicate search result entries from ultimately being returned.


4.2.1.2.1  wholeSubtree search scope


   When the search scope is wholeSubtree, the
   ContinuationReference.searchScope is set to subordinateSubtree.
   Because the ContinuationReference.referrenceType is set to
   nonSpecificSubordinate, the receiving protocol peer allows (and
   expects) name resolution to stop at an immsupr DSE type which is
   treated as a local DSE.  The subordinateSubtree scope instructs the
   receiving protocol peer to exclude the target object from the
   sub-search.


4.2.1.2.2  singleLevel search scope


   When the search scope is singleLevel, and the base DN is resolved to
   a DSE of type nssr, subsequent distributed progressions of the search
   are caused to use the same base DN, and a scope of singleLevel.
   Receiving protocol peers will only apply the search to entries below
   the target object.


   When the search scope is singleLevel and an evaluated DSE is of type
   nssr, no special handling is required.  The search is applied to that
   DSE if it is of type entry.


4.2.1.2.3  baseObject search scope


   No special handling is needed when the search scope is baseObject and
   the base DN is an nssr DSEType.  The search is applied to that DSE if
   it is of type entry.


4.2.1.3  Search operation rooted at an nssr DSE type


   (TODO: a subordinateSubtree scope needs to change to wholeSubtree if
   references are found.)




Sermersheim              Expires August 26, 2005               [Page 18]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



4.3  Populating the ContinuationReference


   When an entry is found to be non-local to a DSA (whether during name
   resolution or operation evaluation), the DSA prepares for operation
   distribution by generating a ContinuationReference.  This is a
   conceptual step, given to help explain the interactions that occur
   between discovering that an operation must be distributing, and
   actually invoking the operation distribution mechanism.
   Implementations are not required to perform this step, but will
   effectively work with the same information.


   After the ContinuationReference has been created, the DSA may choose
   to chain the operation or return a referral (or intermediate
   referral(s)).


   the ContinuationReference is made up of data held on the found
   distributed knowledge information, as well as state information
   gained during name resolution or operation evaluation.


4.3.1  Conveying the Target Object


   The consumer of the ContinuationReference will examine various fields
   in order to determine the target object name of the operation being
   progressed.  The fields examined are the localReference and
   remainingName.


   If name resolution did not complete, and the found distributed
   knowledge information names the same DSE as the base DN of the
   operation, the ContinuationReference MAY omit the localReference
   and/or remainingName fields.


   localReference is populated with the name of the found distributed
   knowledge information DSE.  In the event that the root object holds
   the distributed knowledge information, this field will be populated
   with an empty DN.  Contrast this with the omission of this field.


   referenceType is populated with a value reflecting the reference type
   of the localReference DSE.


   remainingName is populated with the RDNSequence which has not yet
   been resolved.  This is the difference between the localReference
   value and the name of the DSE to be resolved.


   In cases where the DSE named by the {TODO, use a dash or different
   term to make 'found distributed knowledge' more like a single term}
   found distributed knowledge is not the same as the base DN of the
   operation, the ContinuationReference must contain the localReference
   and/or remainingName fields.  Such cases include but are not limited




Sermersheim              Expires August 26, 2005               [Page 19]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



   to:


   o  Distributed knowledge information is found during operation
      evaluation.
   o  Aliases were dereferenced during name resolution.
   o  Name resolution did not complete and there were remaining RDNs to
      be resolved.


4.3.2  Conveying the Remote DSA


   The referralURI field must contain at least one value.  Each
   referralURI value must hold a referralURI.accessPoint.  Other
   requirements on this field as noted may also apply.


   Note for nssr DSE types: During operation evaluation, if a DSE of
   type nssr causes the operation to be distributed (the scenarios in
   Section 4.2.1.2 are an example), then an intermediate referral {TODO:
   this is talking about referral/intermediate referral, but this
   section is only dealing with populating continuation reference} is
   returned for each value of the ref attribute, where each intermediate
   referral only holds a single referralURI value.


4.3.3  Conveying new search scope


   During the evaluation of the search operation, the instructions in
   Section 4.2.1.2.1 and Section 4.2.1.2.2 are followed and the
   searchScope field is updated with the new search scope.


4.3.4  Preventing duplicates


   In order to prevent duplicate entries from being evaluated while
   progressing a search operation, the searchedSubtrees field is
   populated with any naming context below the
   ContinuationReference.targetObject which have been fully searched.


   During the evaluation of the search operation, if the scope is
   wholeSubtree, it is possible that the DSA may search the contents of
   a naming context which is subordinate to another naming context which
   is subordinate to the search base (See figure).













Sermersheim              Expires August 26, 2005               [Page 20]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



                        O X
                       / \
                      /   \
                     /     \
                    /       \
                    \_______O Y
                           /|\
                          / | \
                         /  |  \
                        /   |   \
                       A    B    O C
                                / \
                               /   \
                              /     \
                             /       \
                             \_______/


   In this figure, the DSA holds the naming context X and C,Y,X, but not
   Y,X.  If the search base was X, an intermediate referral would be
   returned for Y,X.  The DSA holding Y,X may also hold a copy of C,Y,X.
   In this case, the receiver of the ContinuationReference benefits by
   knowing that the DSA already searched C,Y,X so that it can prevent
   other DSAs from returning those entries again.


   Data already searched is in the form of an RDNSequence, consisting of
   the RDNs relative to the target object.


4.3.5  Conveying the Failed Name


   At least one DS operation (modifyDN) requires that multiple DNs be
   resolved (the entry being modified and the newSuperior entry).  In
   this case, the failedName field will be populated with the DN being
   resolved which failed name resolution.  This may aid in the
   determination of how the operation is to be progressed.  If both
   names are found to be non-local, this field is omitted.


4.4  Sending a ChainedRequest


   When an entry is found to be non-local to a DSA (whether during name
   resolution or operation evaluation), the DSA may progress the
   operation by sending a chained operation to another DSA (or DSAs).
   The instructions in this section assume that a ContinuationReference
   has been generated which will be used to form the ChainedRequest.  It
   is also assumed that it can be determined whether the operation is
   being progressed due to name resolution or due to operation
   evaluation.


   A DSA which is able to chain operations may advertise this by




Sermersheim              Expires August 26, 2005               [Page 21]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



   returning a value of IANA-ASSIGNED-OID.2; in the supportedFeatures
   attribute on the root DSE.  {TODO: does this and discovery of the
   extended op belong in a new 'discovery mechanisms' sections.}


4.4.1  Forming a ChainedRequest


   The following fields are populated as instructed:


4.4.1.1  ChainedRequestValue.chainingArguments.targetObject


   The ContinuationReference may convey a new target object.  If
   present, the ContinuationReference.localReference field becomes the
   candidate target object.  Otherwise the candidate target object is
   assumed to be that of the original directory operation.  Note that an
   empty value in the ContinuationReference.localReference field denotes
   the root object.


   After performing the above determination as to the candidate target
   object, any RDNSequence in ContinuationReference.remainingName is
   prepended to the determined candidate target object.  This value
   becomes the ChainedRequestValue.chainingArguments.targetObject.  If
   this value matches the value of the original operation, this field
   may be omitted.


4.4.1.2  ChainedRequestValue.chainingArguments.referenceType


   This is populated with the
   ContinuationReference.referralURI.referenceType.


4.4.1.3  ChainedRequestValue.chainingArguments.traceInformation


   This is populated as specified in Section 3.2.1.3.


4.4.1.4  ChainedRequestValue.chainingArguments.searchScope


   This is populated with the
   ContinuationReference.referralURI.searchScope if present, otherwise
   by the ContinuationReference.searchScope if present, and not
   populated otherwise.


4.4.1.5  ChainedRequestValue.chainingArguments.searchedSubtrees


   This is populated with ContinuationReference.searchedSubtrees, as
   well as any previously received values of
   ChainedFinalResponseValue.chainingResults.searchedSubtrees or
   ChainedIntermediateResponseValue.chainingResults.searchedSubtrees
   which are subordinate, relative to the target object.  (If thsi is
   relative to the target object, it can't contain non-relative




Sermersheim              Expires August 26, 2005               [Page 22]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



   subtrees)


4.4.1.6  ChainedRequestValue.operationRequest


   This is populated with the original directory operation request.


4.4.2  Attempting Each Referral URI


   A ContinuationReference consists of one or more referralURIs which
   represent(s a) remote DSA(s).  The chaining DSA attempts to chain to
   each of these DSAs until one succeeds in completing the operation.
   An operation is considered to be completed if it reaches the remote
   DSA and a response is sent back that indicates that the operation was
   executed.  Operations which are sent to the remote DSA, but don't
   complete are indicated by a result code of unavailable or busy.  A
   result code of protocolError may indicate that the DSA does not
   support the chained operation, and in this case, it is also treated
   as an uncompleted operation.  Other errors may in the future specify
   that they also indicate non-completion.  Note that the response may
   itself contain referral(s), these are still considered completed
   operations and thus would subsequently be handled and chained.
   {TODO: could use soft/hard, or transient/permanent
   referral/non-referral error terms here.}


4.4.3  Loop Prevention


   Prior to sending a ChainedRequest, the DSA may attempt to prevent
   looping scenarios by comparing {TODO: what matching rule is used?
   Suggest we don't convert dns names to ip addresses due to NATs} the
   address of the remote DSA and target object to the values of
   ChainedRequestValue.chainingArguments.traceInformation.  If a match
   is found, the DSA returns a loopDetect error.  Note that while this
   type of loop prevention aids in detecting loops prior to sending data
   to a remote DSA, it is not a substitute for loop detection (Section
   Section 4.6.2).  This is because the sending DSA is only aware of a
   single address on which the receiving DSA accepts connections.


4.5  Emulating the Sending of a ChainedRequest


   When it is determined that the operation cannot be distributed by
   means of the ChainedRequest, the chaining DSA may instead emulate the
   steps involved in chaining the operation.  These steps consist of
   performing loop prevention, forming a new directory operation request
   from the original request and possibly updating the base DN, search
   scope, and search filter(in order to emulate searchedSubtrees), and,
   similar to the steps in Section 4.4.2, attempting to send the
   operation request to each DSA listed in the
   ContinuationReference.referralURI until one succeeds in completing




Sermersheim              Expires August 26, 2005               [Page 23]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



   the operation.


   {TODO: We need a way (control) to tell the receiver to allow name
   resolution to end on the parent of a cp (typically an immsupr).  This
   would be sent when the ContinuationReference.referenceType is
   nonSpecificSubordinate}


4.5.1  Emulated Loop Detection


   For this step, the loop prevention instructions in Section 4.4.3 are
   followed.  Note that this method of loop detection may actually allow
   some looping to occur before the loop is detected.


4.5.2  Forming the New Request


   The new directory operation request is formed from the fields of the
   original request, and the following fields may be updated:


   o  The base DN is formed from the new target object as determined by
      following the instructions in Section 4.4.1.1 and using the value
      which would have been placed in
      ChainedRequestValue.chainingArguments.targetObject.
   o  For the search operation, the scope is populated with
      ContinuationReference.searchScope if present, otherwise the scope
      of the original operation request is used.
   o  For the search operation, if the
      ContinuationReference.searchedSubtrees field is present, causes
      the search filter to be augmented by adding a filter item of the
      'and' CHOICE.  The filter consists of {TODO: weasel Kurt into
      finishing his entryDN draft and reference the appropriate section
      there.  See
      <http://www.openldap.org/lists/ietf-ldapext/200407/msg00000.html>
      for context}
   o  Other fields (such as the messageID, and non-critical controls)
      may also need to be updated or excluded.


   If the service being chained to does not support directory
   operations, other operations may be used as long as they provide the
   same level as service as those provided by the analogous directory
   operation.


4.6  Receiving a ChainedRequest


   A DSA which is able to receive and service a ChainedRequest may
   advertise this feature by returning a value of IANA-ASSIGNED-OID.1 in
   the supportedExtension attribute of the root DSE.  {TODO: move?}


   The ChainedRequestValue data type is the requestValue of an




Sermersheim              Expires August 26, 2005               [Page 24]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



   extendedRequest.


   In general, receiving and servicing a ChainedRequest consists of
   performing loop detection and, using components of the
   ChainedRequestType.chainingArguments along with the
   ChainedRequestType.operationRequest, service the request.


4.6.1  Target Object determination


   Prior to checking for a loop condition, the target object must be
   determined.  If the ChainedRequestType.chainingArguments.targetObject
   field is present, its value becomes the target object.  Otherwise,
   the base DN found in the ChainedRequestType.operationRequest becomes
   the target object.


4.6.2  Loop Detection


   The loop detection check happens when a DSA receives a chained
   operation, prior to acting on the operation.  The DSA compares {TODO:
   matching rule? DNS expansion?} each value of
   ChainedRequestValue.traceInformation to the list of addresses at
   which it accepts directory communications.  A value of
   ChainedRequestValue.traceInformation matches when the DSA accepts
   directory communications on the address found in the
   ChainedRequestValue.traceInformation value, and the target object (as
   determined in Section 4.6.1 matches the DN {TODO: using DN matching?}
   value found in the ChainedRequestValue.traceInformation value.  If a
   match is found the DSA returns a loopDetect result.


4.6.3  Processing the ChainedRequestValue.operationRequest


   In processing the operationRequest, the DSA uses the target object
   determined in Section 4.6.1.  For search operations, it uses the
   scope found in ChainedRequestValue.chainingArguments.searchScope, and
   excludes any subtrees relative to the target object indicated in
   ChainedRequestValue.chainingArguments.searchedSubtrees.


   Responses are returned in the form of a Chained Response.


4.7  Returning a Chained Response


   When returning responses to a ChainedRequest, the Chained Response as
   documented in Section 3.3 is used.  If the
   ChainedFinalResponseValue.operationResponse is a searchResultDone,
   the ChainedFinalResponseValue.chainingResults.searchedSubtrees field
   is populated with values consisting of the RDNSequence relative to
   the target object of naming contexts that the DSA searched.  See
   Section 3.3.1.1 for details on why this is done.




Sermersheim              Expires August 26, 2005               [Page 25]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



4.7.1  Chained Response resultCode


   The resultCode for the Chained Response is distinct from the result
   code of the ChainedIntermediateResponseValue.intermediateResponse or
   ChainedFinalResponseValue.finalResponse.  If the act of chaining the
   operation completed, then this value will be success.  Other result
   codes refer to the chained operation itself, and not the result of
   the embedded operation.


4.7.2  Returning referrals in the Chained Response


   {TODO: it would be less complicated if rather than using the simple
   LDAP URL, we used the ContinuationReference type to return referrals
   and intermediate referrals.} {TODO: We need an example of why we
   should allow referrals on a chained response.  Why not just use the
   referral field in the operation?}


4.8  Receiving a Chained Response


   Processing a received Chained Response is generally straight forward
   -- typically the response is simply extracted and returned, but there
   are some extra steps to be taken when chaining sub-operations.


4.8.1  Handling Sub-operation controls and result codes


   When sub-operations are chained, there is the possibility that
   different result codes will be encountered.  Similarly, if controls
   which elicit response controls were attached to the operation, it's
   possible that multiple response controls will be encountered.  Both
   of these possibilities require that the chaining DSA take appropriate
   steps to ensure that the response being returned is correct.


   In general, when a result code indicating an error is received, the
   operation will terminate and the error will be returned.  In cases
   where multiple sub-operations are being concurrently serviced, the
   operation will terminate and the most relevant, or first received
   result code is returned -- determining the result code to be returned
   in this case is a local matter.


   A DSA which chains an operation having a control (or controls)
   attached must ensure that a properly formed response is returned.
   This requires that the DSA understand and know how to aggrigate the
   results of all controls which it allows to remain attached to an
   operation being chained.  If the DSA does not understand or support a
   control which is marked non-critical, it removes the control prior to
   chaining the operation.  The DSA may return
   unavailableCriticalExtension for critical controls that it cannot or
   will not chain.  {TODO: give SSS as an example?}




Sermersheim              Expires August 26, 2005               [Page 26]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



4.8.1.1  Handling referrals during sub-operations


   If a referral is returned in response to a sub-operation, the sending
   DSA may attempt to further chain the operation.  In the event that
   the DSA does not further chain the sub-operation, it will use the
   referral to construct an intermediate referral, and return it
   appropriately.  When using a referral to construct an intermediate
   referral, certain transformations may have to happen.  For example,
   when using a referral to construct a searchResultReference, it must
   be assured that the <dn> field is present, and that the <scope> field
   is properly updated.


4.8.2  Duplicate Elimination


   When search result references cause the DSA to chain a search, it is
   possible that duplicate objects will be returned by different remote
   DSAs.  These duplicate objects must be sensed and not returned.


   {TODO: Even though there are costs associated with returning
   duplicates, is it a worthy exercise to build in an allowance for them
   to be returned? In other words, do we want to add a way for a client
   (or administrator) to say "it's ok, return the duplicates, let the
   client deal with them"? Allowing is seen as a cost benefit to the
   DSA.}


4.9  Returning a Referral or Intermediate Referral


   There are two ways in which the fields of the ContinuationReference
   may be conveyed in a response containing or consisting of referral or
   intermediate referral.  A paired control is introduced for the
   purpose of soliciting and returning a ContinuationReference.  In
   absence of this control, a referral or intermediate referral may be
   returned which conveys the information present in the
   ContinuationReference.  A method of converting a
   ContinuationReference to an LDAP URL is provided for referrals and
   intermediate referrals which identify LDAP-enabled DSAs.  Methods for
   converting a ContinuationReference to URIs which identify non-LDAP
   servers is not provided here, but may be specified in future
   documents, as long as they can represent the data needed to provide
   the same level of service.


4.9.1  ReturnContinuationReference controls


   This control is sent when a client wishes to receive a
   ContinuationReference in the event that a referral or intermediate
   referral is being returned.  If returned, the ContinuationReference
   will hold all data but the referralURI field.  the referralURI values
   will be held in the referral or intermediate referral (Referral,




Sermersheim              Expires August 26, 2005               [Page 27]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



   SearchResultReference, etc.).


4.9.1.1  ReturnContinuationReference request control


   Solicits the return of a ReturnContinuationReference response control
   on messages consisting of (or carrying) a referral or intermediate
   referral.  The controlType is IANA-ASSIGNED-OID.3, the criticality is
   set at the sender's discretion, the controlValue is omitted.


4.9.1.2  ReturnContinuationReference response control


   In response to the ReturnContinuationReference request control, this
   holds a ContinuationReference for messages consisting of (or
   carrying) a referral or intermediate referral.  The controlType is
   IANA-ASSIGNED-OID.3, the controlValue is the BER-encoding of a
   ContinuationReference.  Note that the referralURI field is optionally
   omitted when the ContinuationReference is sent in this control value.
   In this event, the URI(s) found in the referral or intermediate
   referral (Referral, SearchContinuationReference, etc.) are to be used
   in its stead.  {TODO: is returining the referralURI outside an
   unneeded complication?}


4.9.2  Converting a ContinuationReference to an LDAP URL


   This section details the way in which an LDAP URL (from the referral
   or intermediate referral) is used to convey the fields of a
   ContinuationReference.  Where existing LDAP URL fields are
   insufficient, extensions are introduced.  Note that further
   extensions to the ContinuationReference type require further
   specifications here.  {TODO: explain that each ldap url in the
   continuation refrerence is examined and converted}


   These instructions must be applied to each LDAP URL value within the
   referral or intermediate referral.


4.9.2.1  Conveying the target name


   If the <dn> part of the LDAP URL is already present, it is determined
   to be the candidate target object.  Otherwise, the candidate target
   object comes from the ContinuationReference.localReference.  Once the
   candidate target object is determined, the value of
   ContinuationReference.remainingName is prepended to the candidate
   target object.  This new value becomes the target object and its
   string value (as specified by <distinguishedName> in [RFC2253]) is
   placed in the <dn> part of the LDAP URL.







Sermersheim              Expires August 26, 2005               [Page 28]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



4.9.2.2  ContinuationReference.localReference


   This is conveyed as an extension.  The extype is IANA-ASSIGNED-OID.4
   or the descriptor 'localReference', and the exvalue is the string DN
   encoding (as specified by <distinguishedName> in [RFC2253]) of the
   ContinuationReference.localReference value.


4.9.2.3  ContinuationReference.referenceType


   This is conveyed as an extension.  The extype is IANA-ASSIGNED-OID.5
   or the descriptor 'referenceType'.  If the
   ContinuationReference.referenceType is one of superior, subordinate,
   cross, nonSpecificSubordinate, suplier, master, immediateSuperior, or
   self, the exvalue 'superior', 'subordinate', 'cross',
   'nonSpecificSubordinate', 'suplier', 'master', 'immediateSuperior',
   or 'self' respectively.


4.9.2.4  ContinuationReference.searchScope


   If the search scope is one of baseObject, singleLevel, or
   wholeSubtree, then it may be conveyed in the 'scope' part of the LDAP
   URL as 'base', 'one', or 'sub' respectively.  If the search scope is
   subordinateSubtree, then it may be conveyed in the <extension> form
   as documented in [LDAP-SUBORD].  If this extension is present, it
   MUST be marked critical.  This ensures that a receiver which is
   unaware of this extension uses the proper search scope, or fails to
   progress the operation.


4.9.2.5  ContinuationReference.searchedSubtrees


   This field is conveyed as an extension.  The extype is
   IANA-ASSIGNED-OID.6 or the descriptor 'searchedSubtrees', and the
   exvalue is the ContinuationReference.searchedSubtree value encoded
   according to the following searchedSubtrees ABNF:


      searchedSubtrees = 1*(LANGLE searchedSubtree RANGLE)
      searchedSubtree = <distinguishedName> from [RFC2253]
      LANGLE  = %x3C ; left angle bracket ("<")
      RANGLE  = %x3E ; right angle bracket (">")


   Each searchedSubtree represents one RDNSequence value in the
   ContinuationReference.searchedSubtree field.  An example of a
   searchedSubtrees value containing two searched subtrees is:
   <dc=example,dc=com><cn=ralph,dc=users,dc=example,dc=com>.


4.9.2.6  ContinuationReference.failedName


   This field is conveyed as an extension.  The extype is




Sermersheim              Expires August 26, 2005               [Page 29]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



   IANA-ASSIGNED-OID.7 or the descriptor 'failedName', and the exvalue
   is the string DN encoding (as specified in [RFC2253]) of the
   ContinuationReference.failedName value.


4.10  Acting on a Referral or Intermediate Referral


   When a protocol peer receives a referral or intermediate referral, it
   may distribute the operation either by sending a ChainedRequest, or
   by emulating the ChainedRequest.  Prior to taking these steps, the
   protocol peer effectively converts the referral or intermediate
   referral into a ContinuationReference.  Then, acting in the same
   manner as a DSA would, follows the directions in Section 4.4 if
   sending a ChainedRequest, or Section 4.5 otherwise.


4.10.1  Converting a Referral or Intermediate Referral to a
       ContinuationReference


   A referral or intermediate referral may be converted (or conceptually
   converted) to a ContinuationReference type in order to follow the
   distributed operation procedures in Section 4.4, or Section 4.5.  The
   following steps may only be used to convert a referral or
   intermediate referral containing LDAP URL values.  Converting other
   types of URIs may be specified in future documents as long as the
   conversion provides the same level of service found here.


   o  The ContinuationReference.referralURI is populated with all LDAP
      URL values in the referral or intermediate referral.
   o  The ContinuationReference.localReference populate with the value
      of the localReference extension value (Section 4.9.2.2) if one
      exists.  Otherwise it is omitted.
   o  The ContinuationReference.referenceType populate with the value of
      the referenceType extension value (Section 4.9.2.3) if one exists.
      Otherwise it is omitted.
   o  The ContinuationReference.remainingName is omitted.
   o  The ContinuationReference.searchScope is populated with
      subordinateSubtree if the subordScope LDAP URL extension
      [LDAP-SUBORD] is present.  If the <scope> field contains te value
      'base', 'one', 'sub', or 'subordinates', this filed is populated
      with baseObject, singleLevel, wholeSubtree, or subordinateSubtree
      respectively.  Otherwise this field is omitted.
   o  The ContinuationReference.searchedSubtrees is populated with any
      searchedSubtrees LDAP URI extension Section 4.9.2.5 value found on
      an LDAP URI in the referral or intermediate referral.  If none
      exist, this field is omitted.
   o  The ContinuationReference.failedName is populated with any
      failedName LDAP URI extension Section 4.9.2.6 value found on an
      LDAP URI in the referral or intermediate referral.  If none exist,
      this field is omitted.




Sermersheim              Expires August 26, 2005               [Page 30]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



   Note that many fields are simply omitted.  This is either because
   they are conveyed within the LDAP URL values themselves, and
   subsequent instructions will check for their presence, or because
   they are not needed (they are redundant or not used in further
   instructions).


4.11  Ensuring non-existence of an entry under an nssr


   {TODO: add a huge disclaimer here that says without transactional
   semantics, you can never be sure that the entry didn't get added.
   Maybe we should just punt on this and say it's a local matter} In
   order to ensure there are no entries matching the name of the entry
   to be added or renamed immediately subordinate to an nssr, these
   steps may be followed.


   If the DSA is able and allowed to chain operations, it may contact
   each of the DSAs listed as access points in the nssr (in the ref
   attribute) and using a base-level search operation it will determine
   whether or not the object to be added exists.  Note that access
   control or other policies may hide the entry from the sending DSA.
   If the entry does not exist on any of the DSAs listed in the nssr,
   the operation may progress on the local DSA.


   If the DSA cannot make this determination, the operation fails with
   affectsMultipleDSAs.


4.12  Mapping a referralURI to an LDAP URI


   As with any URI specification which is intended to be used as a URI
   which conveys referral information, the LDAP URI specification is
   given a mapping to the elements of a referralURI as specified in.
   Section 3.1.1.1.  These mappings are given here using the ABNF
   identifiers given in [RFC2255].


   referralURI to LDAP URI mapping:


   +---------------------------------+---------------------------------+
   | referralURI element             | LDAP URL element                |
   +---------------------------------+---------------------------------+
   | protocolIdentifier              | <scheme>                        |
   |                                 |                                 |
   | accessPoint                     | <hostport>                      |
   |                                 |                                 |
   | targetObject                    | <dn>. This must be encoded as a |
   |                                 | <distinguishedName> as          |
   |                                 | specified in [RFC2253]          |
   |                                 |                                 |
   | localReference                  | LDAP URL localReference         |




Sermersheim              Expires August 26, 2005               [Page 31]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



   |                                 | extension as specified in       |
   |                                 | Section 4.9.2.2                 |
   |                                 |                                 |
   | referenceType                   | LDAP URL referenceType          |
   |                                 | extension as specified in       |
   |                                 | Section 4.9.2.3                 |
   |                                 |                                 |
   | searchScope                     | <scope> or LDAP URL subordScope |
   |                                 | extension as specified in       |
   |                                 | Section 4.9.2.4                 |
   |                                 |                                 |
   | searchedSubtrees                | LDAP URL searchedSubtrees       |
   |                                 | extension as specified in       |
   |                                 | Section 4.9.2.5                 |
   |                                 |                                 |
   | failedName                      | LDAP URL failedName extension   |
   |                                 | as specified in Section 4.9.2.6 |
   +---------------------------------+---------------------------------+



 4.13   Using the ManageDsaIT control


   This control, defined in [RFC3296], allows the management of the
   distributed knowledge information held by a DSA, and thus overrides
   the determinations made during name resolution and operation
   evaluation.  When this control is attached to an operation, all
   resolved and acted upon DSEs are treated as being local to the DSA.
   This is true regardless of the phase the operation is in.  Thus
   referrals are never returned and chaining never occurs.























Sermersheim              Expires August 26, 2005               [Page 32]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



5.  Security Considerations


   This document introduces a mechanism (chaining) which can be used to
   propagate directory operation requests to servers which may be
   inaccessible otherwise.  Implementers and deployers of this
   technology should be aware of this and take appropriate steps such
   that firewall mechanisms are not compromised.


   This document introduces the ability to return auxiliary data when
   returning referrals.  Measures should be taken to ensure proper
   protection of his data.


   Implementers must ensure that any specified time, size, and
   administrative limits are not circumvented due to the mechanisms
   introduced here.


6.  Normative References


   [LDAP-SUBORD]
              Sermersheim, J., "Subordinate Subtree Search Scope for
              LDAP",
              Internet-Draft draft-sermersheim-ldap-subordinate-scope,
              July 2004.


   [RFC2079]  Smith, M., "Definition of an X.500 Attribute Type and an
              Object Class to Hold Uniform Resource Identifiers (URIs)",
              RFC 2079, January 1997.


   [RFC2119]  Bradner, S., "Key words for use in RFCs to Indicate
              Requirement Levels", BCP 14, RFC 2119, March 1997.


   [RFC2251]  Wahl, M., Howes, T. and S. Kille, "Lightweight Directory
              Access Protocol (v3)", RFC 2251, December 1997.


   [RFC2253]  Wahl, M., Kille, S. and T. Howes, "Lightweight Directory
              Access Protocol (v3): UTF-8 String Representation of
              Distinguished Names", RFC 2253, December 1997.


   [RFC2255]  Howes, T. and M. Smith, "The LDAP URL Format", RFC 2255,
              December 1997.


   [RFC2396]  Berners-Lee, T., Fielding, R. and L. Masinter, "Uniform
              Resource Identifiers (URI): Generic Syntax", RFC 2396,
              August 1998.


   [RFC3296]  Zeilenga, K., "Named Subordinate References in Lightweight
              Directory Access Protocol (LDAP) Directories", RFC 3296,
              July 2002.




Sermersheim              Expires August 26, 2005               [Page 33]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



   [RFC3377]  Hodges, J. and R. Morgan, "Lightweight Directory Access
              Protocol (v3): Technical Specification", RFC 3377,
              September 2002.


   [RFC3383]  Zeilenga, K., "Internet Assigned Numbers Authority (IANA)
              Considerations for the Lightweight Directory Access
              Protocol (LDAP)", BCP 64, RFC 3383, September 2002.


   [RFC3771]  Harrison, R. and K. Zeilenga, "The Lightweight Directory
              Access Protocol (LDAP) Intermediate Response Message",
              RFC 3771, April 2004.


   [X500]     International Telephone and Telegraph Consultative
              Committee, "The Directory - overview of concepts, models
              and services", ITU-T Recommendation X.500, November 1993.


   [X518]     International Telephone and Telegraph Consultative
              Committee, "The Directory - The Directory: Procedures for
              distributed operation", ITU-T Recommendation X.518,
              November 1993.


   [X680]     International Telecommunications Union, "Abstract Syntax
              Notation One (ASN.1): Specification of basic notation",
              ITU-T Recommendation X.680, July 2002.


   [X690]     International Telecommunications Union, "Information
              Technology - ASN.1 encoding rules: Specification of Basic
              Encoding Rules (BER), Canonical Encoding Rules (CER) and
              Distinguished Encoding Rules (DER)", ITU-T Recommendation
              X.690, July 2002.



Author's Address


   Jim Sermersheim
   Novell, Inc
   1800 South Novell Place
   Provo, Utah  84606
   USA


   Phone: +1 801 861-3088
   Email: jimse@novell.com










Sermersheim              Expires August 26, 2005               [Page 34]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



Appendix A.  IANA Considerations


   Registration of the following values is requested [RFC3383].


A.1  LDAP Object Identifier Registrations


   It is requested that IANA register upon Standards Action an LDAP
   Object Identifier in identifying the protocol elements defined in
   this technical specification.  The following registration template is
   provided:


      Subject: Request for LDAP OID Registration
      Person & email address to contact for further information:
         Jim Sermersheim
         jimse@novell.com
      Specification: RFCXXXX
      Author/Change Controller: IESG
      Comments:
      Seven delegations will be made under the assigned OID:
      IANA-ASSIGNED-OID.1 ChainedRequest LDAP Extended Operation
      IANA-ASSIGNED-OID.2 Supported Feature: Can Chain Operations
      IANA-ASSIGNED-OID.3 ReturnContinuationReference LDAP Controls
      IANA-ASSIGNED-OID.4 localReference: LDAP URL Extension
      IANA-ASSIGNED-OID.6 searchedSubtree: LDAP URL Extension
      IANA-ASSIGNED-OID.7 failedName: LDAP URL Extension


A.2  LDAP Protocol Mechanism Registrations


   It is requested that IANA register upon Standards Action the LDAP
   protocol mechanism described in this document.  The following
   registration templates are given:


      Subject: Request for LDAP Protocol Mechanism Registration
      Object Identifier: IANA-ASSIGNED-OID.1
      Description: ChainedRequest LDAP Extended Operation
      Person & email address to contact for further information:
         Jim Sermersheim
         jimse@novell.com
      Usage: Extension
      Specification: RFCXXXX
      Author/Change Controller: IESG
      Comments: none


      Subject: Request for LDAP Protocol Mechanism Registration
      Object Identifier: IANA-ASSIGNED-OID.2
      Description: Can Chain Operations Supported Feature
      Person & email address to contact for further information:





Sermersheim              Expires August 26, 2005               [Page 35]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



         Jim Sermersheim
         jimse@novell.com
      Usage: Feature
      Specification: RFCXXXX
      Author/Change Controller: IESG
      Comments: none


      Subject: Request for LDAP Protocol Mechanism Registration
      Object Identifier: IANA-ASSIGNED-OID.3
      Description: ReturnContinuationReference LDAP Controls
      Person & email address to contact for further information:
         Jim Sermersheim
         jimse@novell.com
      Usage: Control
      Specification: RFCXXXX
      Author/Change Controller: IESG
      Comments: none


      Subject: Request for LDAP Protocol Mechanism Registration
      Object Identifier: IANA-ASSIGNED-OID.4
      Description: localReference LDAP URL Extension
      Person & email address to contact for further information:
         Jim Sermersheim
         jimse@novell.com
      Usage: Extension
      Specification: RFCXXXX
      Author/Change Controller: IESG
      Comments: none


      Subject: Request for LDAP Protocol Mechanism Registration
      Object Identifier: IANA-ASSIGNED-OID.5
      Description: referenceType LDAP URL Extension
      Person & email address to contact for further information:
         Jim Sermersheim
         jimse@novell.com
      Usage: Extension
      Specification: RFCXXXX
      Author/Change Controller: IESG
      Comments: none


      Subject: Request for LDAP Protocol Mechanism Registration
      Object Identifier: IANA-ASSIGNED-OID.6
      Description: searchedSubtree LDAP URL Extension
      Person & email address to contact for further information:
         Jim Sermersheim
         jimse@novell.com
      Usage: Extension





Sermersheim              Expires August 26, 2005               [Page 36]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



      Specification: RFCXXXX
      Author/Change Controller: IESG
      Comments: none


      Subject: Request for LDAP Protocol Mechanism Registration
      Object Identifier: IANA-ASSIGNED-OID.7
      Description: failedName LDAP URL Extension
      Person & email address to contact for further information:
         Jim Sermersheim
         jimse@novell.com
      Usage: Extension
      Specification: RFCXXXX
      Author/Change Controller: IESG
      Comments: none


A.3  LDAP Descriptor Registrations


   It is requested that IANA register upon Standards Action the LDAP
   descriptors described in this document.  The following registration
   templates are given:


      Subject: Request for LDAP Descriptor Registration
      Descriptor (short name): localReference
      Object Identifier: IANA-ASSIGNED-OID.4
      Person & email address to contact for further information:
         Jim Sermersheim
         jimse@novell.com
      Usage: URL Extension
      Specification: RFCXXXX
      Author/Change Controller: IESG
      Comments: none


      Subject: Request for LDAP Descriptor Registration
      Descriptor (short name): referenceType
      Object Identifier: IANA-ASSIGNED-OID.5
      Person & email address to contact for further information:
         Jim Sermersheim
         jimse@novell.com
      Usage: URL Extension
      Specification: RFCXXXX
      Author/Change Controller: IESG
      Comments: none


      Subject: Request for LDAP Descriptor Registration
      Descriptor (short name): searchedSubtree
      Object Identifier: IANA-ASSIGNED-OID.6
      Person & email address to contact for further information:





Sermersheim              Expires August 26, 2005               [Page 37]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



         Jim Sermersheim
         jimse@novell.com
      Usage: URL Extension
      Specification: RFCXXXX
      Author/Change Controller: IESG
      Comments: none


      Subject: Request for LDAP Descriptor Registration
      Descriptor (short name): failedName
      Object Identifier: IANA-ASSIGNED-OID.7
      Person & email address to contact for further information:
         Jim Sermersheim
         jimse@novell.com
      Usage: URL Extension
      Specification: RFCXXXX
      Author/Change Controller: IESG
      Comments: none


A.4  LDAP Result Code Registrations


   It is requested that IANA register upon Standards Action the LDAP
   result codes described in this document.  The following registration
   templates are given:


      Subject: Request for LDAP Result Code Registration
      Result Code Name: invalidReference
      Person & email address to contact for further information:
         Jim Sermersheim
         jimse@novell.com
      Usage: URL Extension
      Specification: RFCXXXX
      Author/Change Controller: IESG
      Comments: none



















Sermersheim              Expires August 26, 2005               [Page 38]
Internet-Draft    Distributed Procedures for LDAP Operations  February 2005



Intellectual Property Statement


   The IETF takes no position regarding the validity or scope of any
   Intellectual Property Rights or other rights that might be claimed to
   pertain to the implementation or use of the technology described in
   this document or the extent to which any license under such rights
   might or might not be available; nor does it represent that it has
   made any independent effort to identify any such rights.  Information
   on the procedures with respect to rights in RFC documents can be
   found in BCP 78 and BCP 79.


   Copies of IPR disclosures made to the IETF Secretariat and any
   assurances of licenses to be made available, or the result of an
   attempt made to obtain a general license or permission for the use of
   such proprietary rights by implementers or users of this
   specification can be obtained from the IETF on-line IPR repository at
   http://www.ietf.org/ipr.


   The IETF invites any interested party to bring to its attention any
   copyrights, patents or patent applications, or other proprietary
   rights that may cover technology that may be required to implement
   this standard.  Please address the information to the IETF at
   ietf-ipr@ietf.org.



Disclaimer of Validity


   This document and the information contained herein are provided on an
   "AS IS" basis and THE CONTRIBUTOR, THE ORGANIZATION HE/SHE REPRESENTS
   OR IS SPONSORED BY (IF ANY), THE INTERNET SOCIETY AND THE INTERNET
   ENGINEERING TASK FORCE DISCLAIM ALL WARRANTIES, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO ANY WARRANTY THAT THE USE OF THE
   INFORMATION HEREIN WILL NOT INFRINGE ANY RIGHTS OR ANY IMPLIED
   WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.



Copyright Statement


   Copyright (C) The Internet Society (2005).  This document is subject
   to the rights, licenses and restrictions contained in BCP 78, and
   except as set forth therein, the authors retain all their rights.



Acknowledgment


   Funding for the RFC Editor function is currently provided by the
   Internet Society.





Sermersheim              Expires August 26, 2005               [Page 39]