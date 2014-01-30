# Xpath functions
The following xpath functions were added to the engine:
* 'revXMLEvaluateXPath'
* 'revXMLDataFromXPathQuery'

In addition, several synonyms for existing commands are now available for consistency:
* 'revXMLCreateTree'
* 'revXMLCreateTreeWithNamespaces'
* 'revXMLCreateTreeFromFile'
* 'revXMLCreateTreeFromFileWithNamespaces'
* 'revXMLDeleteTree'
* 'revXMLAppend'
* 'revXMLDeleteAllTrees'
* 'revXMLAddNode'
* 'revXMLDeleteNode'
* 'revXMLInsertNode'
* 'revXMLMoveNode'
* 'revXMLCopyNode'
* 'revXMLCopyRemoteNode'
* 'revXMLMoveRemoteNode'
* 'revXMLPutIntoNode'
* 'revXMLSetAttribute'

## Syntax
    revXMLEvaluateXPath(pDocID, pXpathExpression [, delimiter])
    revXMLDataFromXPathQuery(pDocID, pXpathExpression [, delimiter])

## Usage
Given tXMLData as

    <?xml version="1.0" encoding="ISO-8859-1"?>
    <bookstore>
        <book category="COOKING">
          <title lang="en">Everyday Italian</title>
          <author>Giada De Laurentiis</author>
          <year>2005</year>
          <price>30.00</price>
        </book>
        <book category="CHILDREN">
          <title lang="en">Harry Potter</title>
          <author>J K. Rowling</author>
          <year>2005</year>
          <price>29.99</price>
        </book>
        <book category="WEB">
          <title lang="en">XQuery Kick Start</title>
          <author>James McGovern</author>
          <author>Per Bothner</author>
          <author>Kurt Cagle</author>
          <author>James Linn</author>
          <author>Vaidyanathan Nagarajan</author>
          <year>2003</year>
          <price>49.99</price>
        </book>
        <book category="WEB">
          <title lang="en">Learning XML</title>
          <author>Erik T. Ray</author>
          <year>2003</year>
          <price>39.95</price>
        </book>
    </bookstore>

Then:

    put revXMLCreateTree(tXMLData,false,true,false) into pDocID
    put "/bookstore/book[price<50]" into pXpathExpression
    put revEvaluateXPath(pDocID, pXpathExpression)

Gives you:

    /bookstore/book[1]
    /bookstore/book[2]
    /bookstore/book[3]
    /bookstore/book[4]

And:

    put "/bookstore/book[price<30]/title" into pXpathExpression
    put revDataFromXPathQuery(pDocID, pXpathExpression)

Gives you 'Harry Potter'

