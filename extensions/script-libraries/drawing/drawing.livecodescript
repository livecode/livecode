script "com.livecode.library.drawing"
/*
Copyright (C) 2017 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

on extensionInitialize
   if the target is not me then
      pass extensionInitialize
   end if

   insert the script of me into back

   if the environment contains "development" then
      set the _ideoverride of me to true
      local tName
      put the short name of me into tName
      revSBAddDependencyForInclusion "extensions", tName, "externals", "XML"
   end if

   svgSpecLoad
end extensionInitialize

on extensionFinalize
   if the target is not me then
      pass extensionFinalize
   end if

   remove the script of me from back
end extensionFinalize

/**
Title: Drawing Library

Author: LiveCode

Version: 0.0.0

Description:
This script library implements an SVG compiler for turning SVG files into
'drawing metafiles'. A simple and efficient binary format for representing
complex vector graphics, supported directly by the LiveCode image object.
*/

/**
Compile an SVG XML file to a drawing metafile.

Example:
on mouseUp
	set the text of image 1 to drawingSvgCompileFile("clock.svg")
end mouseUp

Description:
Use <drawingSvgCompileFile> to build a binary string representing an SVG file
which can be used as the 'text' of an image object for display.

The SVG XML file is parsed using revXML and then stripped of all attributes
and elements not currently understood by the SVG compiler. The resulting SVG
is then converted to a lower-level form and encoded. This form can be used
as the text of an image.

The following SVG features are currently supported:

  - 'svg' elements (percentage width/height only allowed on the root element)
  - 'g' and 'defs' elements
  - 'use' elements
  - 'rect', 'circle', 'ellipse', 'line', 'polyline', 'polygon' and 'path' elements
  - 'fill', 'fill-opacity', fill-rule' properties
  - 'stroke', 'stroke-opacity', stroke-width', 'stroke-dash-array',
    'stroke-dash-offset', 'stroke-line-cap', 'stroke-line-join' and
    'stroke-miter-limit' properties
  - 'solidColor', 'linearGradient', 'radialGradient' paint servers
  - 'solid-color' and 'solid-opacity' solid color server properties
  - 'stop-color' and 'stop-opacity' gradient ramp properties
  - absolute unit specifiers in, cm, mm, pt, pc, px

Color values of the following forms are supported:

  - &#35;rgb
  - &#35;rrggbb
  - rgb(rrr, ggg, bbb)
  - standard SVG 1.1 named colors (https://www.w3.org/TR/SVG/types.html#ColorKeywords)

Radial gradients support focal point (fx, fy) and SVG2's focal radius (fr).

The rendering of an SVG file inside an image object respects the width, height,
viewBox and preserveAspectRatio attributes on the root SVG node in the document.

If the width and height attributes are specified, and they are not percentages,
then they are taken to be the formattedWidth/Height of the image object. In this
case the SVG will always display at that fixed size, clipped to the rect of the
image.

If the width and height attributes are not specified, or are percentages, and
a viewBox attribute is specified then the width and height of the viewBox are
taken to be the formattedWidth/Height of the image object. In this case the SVG
will scale to fit within the rect of the image object, respecting the setting
of the SVG's preserveAspectRatio attribute.

Finally if the width and height are not specified, or are percentages and there
is no viewBox attribute then the intrinsic width and height are taken to be
256.

>*Note:* The drawing binary format is not currently considered stable and is
subject to change until the end of the RC cycle for 9. At present it is advised
that SVG files be compiled as needed when developing in the IDE, and then
compiled ahead-of-time when building a standalone.


>*Note:* To use this function in a standalone, you must include the XML 
extension.


>*Note:* The <XML library> (and by extension, this function) can not load 
> files from the resources folder on Android. You must either copy the 
> resources to the documents folder or use the <drawingSvgCompile>
> function instead.

Parameters:

pXmlFile (string):
The filename of the SVG XML file to load.

Returns (data):
A binary string containing the encoded drawing - this can be used as the text
of an image directly.

The result:
An error string if an error occurred.

References: drawingSvgCompile (function), XML library (library)
*/
function drawingSvgCompileFile pXmlFile
	/* Convert the XML file to the LiveCode array structure. */
	local tArray

	try
		lock screen
		put xmlImportFromFile(pXmlFile) into tArray
		return svgImportFromArray(tArray) for value
	catch tError
		return tError for error
	finally
		unlock screen
	end try
end drawingSvgCompileFile

/**
Compile an SVG XML file to a drawing metafile.

Example:
on mouseUp
	set the text of image 1 to drawingSvgCompile(url ("file:clock.svg"))
end mouseUp

Description:
Use <drawingSvgCompile> to build a binary string representing an SVG file
which can be used as the 'text' of an image object for display.

The text of the SVG XML document is parsed using revXML and then stripped of all
attributes and elements not currently understood by the SVG compiler. The
resulting SVG is then converted to a lower-level form and encoded. This form can
be used as the text of an image.

The following SVG features are currently supported:

  - 'svg' elements (percentage width/height only allowed on the root element)
  - 'g' and 'defs' elements
  - 'use' elements
  - 'rect', 'circle', 'ellipse', 'line', 'polyline', 'polygon' and 'path' elements
  - 'fill', 'fill-opacity', fill-rule' properties
  - 'stroke', 'stroke-opacity', stroke-width', 'stroke-dash-array',
    'stroke-dash-offset', 'stroke-line-cap', 'stroke-line-join' and
    'stroke-miter-limit' properties
  - 'solidColor', 'linearGradient', 'radialGradient' paint servers
  - 'solid-color' and 'solid-opacity' solid color server properties
  - 'stop-color' and 'stop-opacity' gradient ramp properties
  - absolute unit specifiers in, cm, mm, pt, pc, px

Color values of the following forms are supported:

  - &#35;rgb
  - &#35;rrggbb
  - rgb(rrr, ggg, bbb)
  - standard SVG 1.1 named colors (https://www.w3.org/TR/SVG/types.html#ColorKeywords)

Radial gradients support focal point (fx, fy) and SVG2's focal radius (fr).

The rendering of an SVG file inside an image object respects the width, height,
viewBox and preserveAspectRatio attributes on the root SVG node in the document.

If the width and height attributes are specified, and they are not percentages,
then they are taken to be the formattedWidth/Height of the image object. In this
case the SVG will always display at that fixed size, clipped to the rect of the
image.

If the width and height attributes are not specified, or are percentages, and
a viewBox attribute is specified then the width and height of the viewBox are
taken to be the formattedWidth/Height of the image object. In this case the SVG
will scale to fit within the rect of the image object, respecting the setting
of the SVG's preserveAspectRatio attribute.

Finally if the width and height are not specified, or are percentages and there
is no viewBox attribute then the intrinsic width and height are taken to be
256.

>*Note:* The drawing binary format is not currently considered stable and is
> subject to change until the end of the RC cycle for 9. At present it is advised
> that SVG files be compiled as needed when developing in the IDE, and then
> compiled ahead-of-time when building a standalone.


>*Note:* To use this function in a standalone, you must include the XML 
> extension.


Parameters:

pXmlText (string):
The text of the SVG XML document to compile.

Returns (data):
A binary string containing the encoded drawing - this can be used as the text
of an image directly.

The result:
An error string if an error occurred.

References: drawingSvgCompileFile (function)
*/
function drawingSvgCompile pXmlText
	/* Convert the XML file to the LiveCode array structure. */
	local tArray

	try
		lock screen
		put xmlImportFromText(pXmlText) into tArray
		return svgImportFromArray(tArray) for value
	catch tError
		return tError for error
	finally
		unlock screen
	end try
end drawingSvgCompile

private function svgImportFromArray pXmlArray
	local tArray
	put pXmlArray into tArray

	/* Processing of the input documents runs using a 'context' array. This
	 * is threaded through all the recursive processing functions, holding
	 * information about what is being processed (for error handling) and also
	 * any state which is computed as the processing phrases progress. */
	local tContext
	put empty into tContext

	/* First process the svg array, transforming it to a state it can be
	 * compiled. Each process operation takes tArray as a reference parameter
	 * so they can update the array in place. */

	/* Remove all elements from the tree which are not understood. */
	_svgTrim tContext, tArray

	/* Parse the values of all features, including those specified in any inline
	 * style attributes. Any features which are not applicable to the specifying
	 * element are removed, and any required attributes are set with their
	 * default values (required, unset properties are set during cascade). */
	_svgParse tContext, tArray

	/* Create a map from element id attribute to label. */
	_svgMap tContext, tArray

	/* Flatten all 'use' elements, replacing them with their referenced
	 * elements. */
	_svgFlatten tContext, tArray

	/* Perform the property cascade, assigning appropriate inherited values
	 * to all properties which apply to a given element. */
	local tPropertyDefaults
	put svgSpecGetPropertyDefaults() into tPropertyDefaults
	repeat for each key tPropertyName in tPropertyDefaults
		get _svgParseFeatureValue(tContext, "", tPropertyName, tPropertyDefaults[tPropertyName])
	end repeat
	_svgCascade tContext, tPropertyDefaults, tArray

	/* Remap as the previous two phases will have mutated the tree */
	put empty into tContext["ids"]
	_svgMap tContext, tArray

	/* Now all processing has been done we can perform the final phase of
	 * compiling the svg document. */

	/* Compile the processed array, converting its tree structure into a linear
	 * sequence of graphics operations. */
	_svgCompile tContext, tArray


	/* Encode the operations array in a form suitable for processing by LCB */
	local tDrawing
	_svgEncode tContext, tDrawing

	return tDrawing
end svgImportFromArray

/*******************************************************************************
 *
 *  SVG PROCESS OPERATIONS
 *
 ******************************************************************************/

/* _svgTrim iterates over the document array, removing any elements which are
 * are not understood. */
private command _svgTrim @xContext, @xElement
	local tElementType
	put xElement["type"] into tElementType

	/* Push the node's path segment onto the current context so errors can be
	 * adequately placed. */
	_svgContextEnter xContext, xElement

	/* If the element type is known, we recurse to its children then process
	 * the resulting sequence to remove holes */
	if tElementType is empty then
		put empty into xElement
	else if svgSpecIsElement(tElementType) then
		if xElement["content"] is an array then
			local tNewContent
			repeat for each element tChildElement in xElement["content"]
				/* If a node type is unknown, then _svgTrim will set
				 * tChildElement to empty. */
				_svgTrim xContext, tChildElement

				/* If the node survived the cut, then repush it onto a new
				 * sequence - this removes any holes. */
				if tChildElement is an array then
					seqPushOntoBack tNewContent, tChildElement
				end if
			end repeat
			/* Replace the original content of the element with the new content. */
			put tNewContent into xElement["content"]
		end if
	else
		_svgContextUnknownElementError xContext, tElementType
		put empty into xElement
	end if

	/* Pop the node's path segment from the current context. */
	_svgContextLeave xContext, xElement
end _svgTrim

/* _svgParse iterates over the document tree, parsing feature values on each
 * element. Any inline style features are expanded and the resulting feature
 * values are parsed. Any attributes which are not applicable to the current
 * element are removed, but properties remain as they will cascade to child
 * elements in a later processing phase. Finally, any missing non-nullable
 * attributes are set with their default value. */
private command _svgParse @xContext, @xElement
 	local tElementType
	put xElement["type"] into tElementType

	/* Push the element's path segment onto the current context so errors can be
	 * adequately reported. */
	_svgContextEnter xContext, xElement

	/* Iterate through all features on the element, parsing the values of each
	 * one. If a feature is not settable on the element it is removed. If a
	 * feature's value fails to parse then it is removed.
	 * Note: In general, SVG treats invalid feature values as if the feature is
	 * not explicitly present; this allows the defaulting mechanism to take
	 * place in that instance. */
	repeat for each key tFeatureName in xElement["features"]
		/* Features which are not settable on the current element are
		 * removed. */
		if not svgSpecIsFeatureSettableOnElement(tFeatureName, tElementType) then
			delete variable xElement["features"][tFeatureName]
			_svgContextUnknownFeatureError xContext, tFeatureName, tElementType
			next repeat
		end if

		/* Parse the feature's value. If the parsing fails, then it is removed. */
		if not _svgParseFeatureValue(xContext, tElementType, tFeatureName, xElement["features"][tFeatureName]) then
			delete variable xElement["features"][tFeatureName]
			_svgContextInvalidValueForFeatureError xContext, tFeatureName, tElementType
			next repeat
		end if
	end repeat

	/* If the element has a 'style' attribute, then process that now. The style
	 * string is parsed, any properties which are not settable on the element
	 * are removed and the rest have their values parsed. Any properties which
	 * are successfully parsed in this manner are then added as properties to the
	 * element (this is because properties specified in inline style attributes
	 * take precedence over those which are specified in the element). */
	if "style" is among the keys of xElement["features"] then
		local tStyleProperties
		put xElement["features"]["style"] into tStyleProperties
		if _svgParseStyleAttributeValue(tStyleProperties) then
			repeat for each key tPropertyName in tStyleProperties
				/* Properties which are not settable to the current element are
				 * ignored. */
				if not svgSpecIsPropertySettableOnElement(tPropertyName, tElementType) then
					_svgContextUnknownStylePropertyError xContext, tPropertyName, tElementType
					next repeat
				end if

				/* Parse the property's value. If parsing fails, then the property
				 * is ignored. */
				if not _svgParseFeatureValue(xContext, tElementType, tPropertyName, tStyleProperties[tPropertyName]) then
					_svgContextInvalidValueForStylePropertyError xContext, tPropertyName, tElementType
					next repeat
				end if

				/* Apply the property to the element's features. */
				put tStyleProperties[tPropertyName] into xElement["features"][tPropertyName]
			end repeat
		else
			_svgContextInvalidValueForStyleError xContext
		end if

		/* The style attribute has been processed, so can now be removed. */
		delete variable xElement["features"]["style"]
	end if

	/* Iterate through all applicable attributes for an element, and ensure
	 * that all are defined (applying the default value of any which have not
	 * been specified). */
	repeat for each key tAttributeName in svgSpecGetApplicableAttributesOfElement(tElementType)
		/* If the attribute has already been specified on the node, then there
		 * is nothing to do. */
		if tAttributeName is among the keys of xElement["features"] then
			next repeat
		end if

		/* If the attribute is not required, then there is nothing to do. */
		if not svgSpecIsAttributeRequiredForElement(tElementType, tAttributeName) then
			next repeat
		end if

		/* Set the attribute to its default value. */
		put svgSpecGetAttributeDefaultForElement(tElementType, tAttributeName) into xElement["features"][tAttributeName]

		/* Parse the default value (which should never be an error...). */
		if not _svgParseFeatureValue(xContext, tElementType, tAttributeName, xElement["features"][tAttributeName]) then
			_svgContextInvalidDefaultValueError xContext, tAttributeName, tElementType
			delete variable xElement["features"][tAttributeName]
		end if
	end repeat

	/* Now that the element has been processed, process the features of each
	 * child element (if any). */
	if xElement["content"] is an array then
		repeat with tIndex = 1 to the number of elements in xElement["content"]
			_svgParse xContext, xElement["content"][tIndex]
		end repeat
	end if

	/* Pop the element's path segment from the context. */
	_svgContextLeave xContext, xElement
end _svgParse

/* _svgMap creates a mapping from id to element with that id so that hrefs
 * within the document can be resolved. */
private command _svgMap @xContext, @xElement
	/* Push the node's path segment onto the current context so errors can be
	 * adequately placed. */
	_svgContextEnter xContext, xElement

	local tId
	if "id" is among the keys of xElement["features"] then
		put xElement["features"]["id"] into tId
	else if "xml:id" is among the keys of xElement["features"] then
		put xElement["features"]["xml:id"] into tId
	end if

	if tId is not empty then
		if not _svgContextDefine(xContext, tId, xElement) then
			_svgContextElementAlreadyDefinedError xContext, xElement["features"]["id"]
		end if
	end if

	/* Now all the processing for this node has been done, we to process each
	 * child, in order. */
	if xElement["content"] is an array then
		repeat with tIndex = 1 to the number of elements in xElement["content"]
			_svgMap xContext, xElement["content"][tIndex]
		end repeat
	end if

	/* Pop the node's path segment from the current context. */
	_svgContextLeave xContext, xElement
end _svgMap

/* _svgFlatten deep copies all used elements into their place in the document.
 * Note: Due to LiveCode's copy-on-write semantics, this deep-copying doesn't
 * actually cost anything. */
private command _svgFlatten @xContext, @xElement
	/* Push the node's path segment onto the current context so errors can be
	 * adequately placed. */
	_svgContextEnter xContext, xElement

	/* Only process 'use' nodes. */
	if xElement["type"] is "use" then
		/* Fetch the id from the xlink:href attribute. A missing attribute, or
		 * an empty id result in nothing happening. */
		local tId
		put xElement["features"]["xlink:href"] into tId
		if tId is empty then
			put xElement["features"]["href"] into tId
		end if
		if tId is not empty then
			local tReferencedElement
			if _svgContextLookup(xContext, tId, tReferencedElement) then
				/* Flatten the referenced element, this will flatten any 'use'
				 * elements within it.
				 * Note: This currently does not guard against circularity! */
				_svgFlatten xContext, tReferencedElement

				/* A 'use' node is turned into a 'g' node with the referenced
				 * elements content. */
				put tReferencedElement into xElement["content"][1]
				put "g" into xElement["type"]

				/* Apply a translation defined by the x, y attributes on the use
				 * element. */
				local tTransform
				put "translate" into tTransform[1]
				put xElement["features"]["x"] into tTransform[2]
				put xElement["features"]["y"] into tTransform[3]
				seqPushOntoBack xElement["features"]["transform"], tTransform

				/* Remove any attributes which are specific to the use node. */
				delete variable xElement["features"]["x"]
				delete variable xElement["features"]["y"]
				delete variable xElement["features"]["xml:base"]
				delete variable xElement["features"]["xlink:href"]
				delete variable xElement["features"]["href"]
			else
				_svgContextElementNotDefinedError xContext, tId
			end if
		end if
	end if

	/* Now all the processing for this element has been done, we process each
	 * child, in order. */
	if xElement["content"] is an array then
		repeat with tIndex = 1 to the number of elements in xElement["content"]
			_svgFlatten xContext, xElement["content"][tIndex]
		end repeat
	end if

	/* Pop the node's path segment from the current context. */
	_svgContextLeave xContext, xElement
end _svgFlatten

/* _svgProcessCascade performs the CSS-style cascade of property values down
 * through the elements. */
private command _svgCascade @xContext, pProperties, @xElement
	local tElementType
	put xElement["type"] into tElementType

	/* Push the element's path segment onto the current context so errors can be
	 * adequately placed. */
	_svgContextEnter xContext, xElement

	/* Iterate through all the properties and ensure that all which are applicable
	 * to the element are set on the element. If an applicable property is not
	 * set on an element, then it is either set to the inherited property (if
	 * the property is inheritable), or to the property default (if the property
	 * is not inheritable). If an applicable property on an element has the value
	 * 'inherit', then it is updated with the parent property value; otherwise
	 * the properties array passed to child elements is updated with the new
	 * value. */
	repeat for each key tPropertyName in pProperties
		if svgSpecIsPropertyApplicableToElement(tPropertyName, tElementType) then
			if tPropertyName is not among the keys of xElement["features"] then
				if svgSpecIsPropertyInheritable(tPropertyName) then
					put pProperties[tPropertyName] into xElement["features"][tPropertyName]
				else
					put svgSpecGetDefaultValueForProperty(tPropertyName) into xElement["features"][tPropertyName]
					get _svgParseFeatureValue(xContext, empty, tPropertyName, xElement["features"][tPropertyName])
					put xElement["features"][tPropertyName] into pProperties[tPropertyName]
				end if
			else if xElement["features"][tPropertyName] is "inherit" then
				put pProperties[tPropertyName] into xElement["features"][tPropertyName]
			else
				put xElement["features"][tPropertyName] into pProperties[tPropertyName]
			end if
		else
			/* The property is not directly applicable to the element, but if it
			 * is set on it and it is not 'inherit' then update the properties
			 * array to pass to child elements. */
			if tPropertyName is among the keys of xElement["features"] and \
				xElement["features"][tPropertyName] is not "inherit" then
				put xElement["features"][tPropertyName] into pProperties[tPropertyName]
				delete variable xElement["features"][tPropertyName]
			end if
		end if
	end repeat

	/* Now all the processing for this node has been done, we to process each
	 * child, in order. */
	if xElement["content"] is an array then
		repeat with tIndex = 1 to the number of elements in xElement["content"]
			_svgCascade xContext, pProperties, xElement["content"][tIndex]
		end repeat
	end if

	/* Pop the node's path segment from the current context. */
	_svgContextLeave xContext, xElement
end _svgCascade

/*******************************************************************************
 *
 *  SVG COMPILE OPERATIONS
 *
 ******************************************************************************/

private command _svgCompile @xContext, pRootElement
	/* If the root element isn't an 'svg' then there's not much we can do! */
	if pRootElement["type"] is not "svg" then
		throw "root element is not svg"
	end if

	/* Unpack the root element's attributes. */
	local tWidth, tHeight, tViewBox, tPar
	put pRootElement["features"]["width"] into tWidth
	put pRootElement["features"]["height"] into tHeight
	put pRootElement["features"]["viewBox"] into tViewBox
	put pRootElement["features"]["preserveAspectRatio"] into tPar

	/* At this point we might find:
	 *   1) Absolute width/height
	 *   2) Percent width/height
	 *   3) A viewBox
	 *
	 * We need an absolute width/height to present as size of the drawing. This
	 * is used to determine how things can scale, and the 'natural' mapping to
	 * pixels.
	 *
	 * If the width/height are relative (they default to 100% if not specified)
	 * then they are taken as percentage sizes of the viewBox width/height.
	 *
	 * If there is no viewBox in that case, then the viewBox is taken to be
	 * the point bbox of all non-relatively specified elements, or a fixed size
	 * of 256 if that bbox is empty.
	 */

	/* If we have a viewBox, then all is straight-forward. We use that to
	 * generate absolute width/height. Otherwise, if the width/height are relative
	 * we must do bbox computations. */
	if tViewBox is an array then
		if tWidth < 0 then
			put -tWidth * tViewBox[3] into tWidth
		end if

		if tHeight < 0 then
			put -tHeight * tViewBox[4] into tHeight
		end if
	else if tWidth < 0 or tHeight < 0 then
		throw "relative width/height with no viewBox not supported yet"
	end if

	/* At this point we have an absolute width/height, so update the features
	 * of the root element. */
	put tWidth into pRootElement["features"]["width"]
	put tHeight into pRootElement["features"]["height"]

	/* Set the initial root-width/height. */
	put tWidth into xContext["root-width"]
	put tHeight into xContext["root-height"]

	/* Set the drawing size */
	put tWidth into xContext["drawing-width"]
	put tHeight into xContext["drawing-height"]

	_svgCompileElement xContext, pRootElement
end _svgCompile

private command _svgCompileElement @xContext, pElement
	/* Skip any elements which don't require compilation */
	switch pElement["type"]
	case "defs"
	case "use"
	case "solidColor"
	case "linearGradient"
	case "radialGradient"
		exit _svgCompileElement
	end switch

	/* Save the current feature state */
	_svgContextSave xContext

	/* Compile features into the feature state. */
	_svgCompileFeatures xContext, pElement

	/* Dispatch to appropriate compile operations based on element type. */
	switch pElement["type"]
	case "svg"
		_svgCompileSvg xContext, pElement
		break
	case "g"
		repeat for each element tChildElement in pElement["content"]
			_svgCompileElement xContext, tChildElement
		end repeat
		break
	case "rect"
		_svgCompileRectangle xContext, pElement
		break
	case "circle"
		_svgCompileCircle xContext, pElement
		break
	case "ellipse"
		_svgCompileEllipse xContext, pElement
		break
	case "line"
		_svgCompileLine xContext, pElement
		break
	case "polyline"
		_svgCompilePolyline xContext, pElement
		break
	case "polygon"
		_svgCompilePolygon xContext, pElement
		break
	case "path"
		_svgCompilePath xContext, pElement
		break
	default
		_InternalError format("unhandled element type '%s'", pElement["type"])
		break
	end switch

	/* Restore the feature state */
	_svgContextRestore xContext
end _svgCompileElement

/* _svgCompileFeatures loops through all features defined on element and sets
 * the feature state of the context to their 'compiled' values. The compiled
 * values for the features are such that equivalence can be checked by
 * simple equality. */
private command _svgCompileFeatures @xContext, pElement
	repeat for each key tFeatureName in pElement["features"]
		local tFeatureValue
		put empty into tFeatureValue

		switch tFeatureName
		case "transform"
			local tTransform
			put _svgContextGetState(xContext, "transform") into tTransform
			seqAppend tTransform, pElement["features"]["transform"]
			_svgCompileTransform xContext, tTransform, empty, tFeatureValue
			break
		case "fill"
		case "stroke"
			_svgCompilePaint xContext, pElement["features"][tFeatureName], tFeatureValue
			break
		case "fill-opacity"
		case "fill-rule"
		case "stroke-opacity"
		case "stroke-width"
		case "stroke-linejoin"
		case "stroke-linecap"
		case "stroke-miterlimit"
		case "stroke-dashoffset"
		case "stroke-dasharray"
			put pElement["features"][tFeatureName] into tFeatureValue
			break
		default
			/* Ignore any features which aren't pertinent to compilation */
			next repeat
		end switch
		_svgContextSetState xContext, tFeatureName, tFeatureValue
	end repeat
end _svgCompileFeatures

private command _svgCompileTransform @xContext, pTransform, pBBox, @rCompiledTransform
	local tMatrix
	put _svgTransformIdentity() into tMatrix

	repeat for each element tTransform in pTransform
		local tNextMatrix
		switch tTransform[1]
		case "matrix"
			put _svgTransformMatrix(tTransform[2], tTransform[3], tTransform[4], tTransform[5], tTransform[6], tTransform[7]) into tNextMatrix
			break
		case "translate"
			put _svgTransformTranslate(tTransform[2], tTransform[3]) into tNextMatrix
			break
		case "scale"
			put _svgTransformScale(tTransform[2], tTransform[3]) into tNextMatrix
			break
		case "rotate"
			put _svgTransformTranslate(tTransform[3], tTransform[4]) into tNextMatrix
			put _svgTransformConcat(tNextMatrix, _svgTransformRotate(tTransform[2])) into tNextMatrix
			put _svgTransformConcat(tNextMatrix, _svgTransformTranslate(-tTransform[3], -tTransform[4])) into tNextMatrix
			break
		case "skewX"
			put _svgTransformSkew(tTransform[2], 0) into tNextMatrix
			break
		case "skewY"
			put _svgTransformSkew(0, tTransform[2]) into tNextMatrix
			break
		case "bbox"
			if pBBox is not an array then
				_InternalError "compile transform called for bbox transform with no bbox"
			end if
			put _svgTransformMatrix(pBBox["right"] - pBBox["left"], 0, 0, pBBox["bottom"] - pBBox["top"], pBBox["left"], pBBox["top"]) into tNextMatrix
			break
		default
			_InternalError format("unknown transform '%s'", tTransform[1])
			break
		end switch
		put _svgTransformConcat(tMatrix, tNextMatrix) into tMatrix
	end repeat
	put empty into rCompiledTransform
	if not _svgTransformIsIdentity(tMatrix) then
		seqPushOntoBack rCompiledTransform, _svgTransformFlatten(tMatrix)
	end if
end _svgCompileTransform

private command _svgCompilePaint @xContext, pPaint, @rCompiledPaint
	put empty into rCompiledPaint
	switch pPaint["type"]
	case "none"
		put "none" into rCompiledPaint[1]
		break
	case "current"
		put "current" into rCompiledPaint[1]
		break
	case "color"
		put "color" into rCompiledPaint[1]
		put pPaint["red"] into rCompiledPaint[2]
		put pPaint["green"] into rCompiledPaint[3]
		put pPaint["blue"] into rCompiledPaint[4]
		put pPaint["alpha"] into rCompiledPaint[5]
		break
	case "reference"
		_svgCompilePaintServer xContext, pPaint["id"], pPaint["fallback"], rCompiledPaint
		break
	default
		put "none" into rCompiledPaint[1]
	end switch
end _svgCompilePaint

private command _svgCompilePartialPaint @xContext, @xPaint, pBBox
	switch xPaint[1]
	case "linear"
	case "radial"
	case "conical"
		_svgCompileTransform xContext, xPaint[3], pBBox, xPaint[3]
		break
	default
		break
	end switch
end _svgCompilePartialPaint

private command _svgCompilePaintServer @xContext, pId, pFallback, @rCompiledPaint
	local tServerElement
	if _svgContextLookup(xContext, pId, tServerElement) then
		switch tServerElement["type"]
		case "solidColor"
			local tSolidColor, tSolidOpacity
			_svgCompilePaint xContext, tServerElement["features"]["solid-color"], rCompiledPaint
			multiply rCompiledPaint[5] by tServerElement["features"]["solid-opacity"]
			return empty
		case "linearGradient"
		case "radialGradient"
			local tGradient
			_svgCompileResolveGradient xContext, tServerElement, tGradient
			if tServerElement["type"] is "linearGradient" then
				put "linear" into rCompiledPaint[1]
			else
				put "radial" into rCompiledPaint[1]
			end if
			put tGradient["spreadMethod"] into rCompiledPaint[2]
			if tGradient["gradientUnits"] is "objectBoundingBox" then
				put "bbox" into rCompiledPaint[3][1][1]

				/* Percentage values in the coordinates of the gradient are
				 * relative to the bbox */
				repeat for each item tCoordKey in "x1,x2,y1,y2,r,cx,cy,fr,fx,fy"
					if tGradient[tCoordKey] ends with "%" then
						put (char 1 to -2 of tGradient[tCoordKey]) / 100 into tGradient[tCoordKey]
					end if
				end repeat
			else
				/* Percentage values in the x coords are relative to the viewport
				 * width. */
				repeat for each item tCoordKey in "x1,x2,cx,fx"
					if tGradient[tCoordKey] ends with "%" then
						if xContext["root-width"] < 0 then
							throw "viewport relative grad coords not supported"
						end if
						put xContext["root-width"] * (char 1 to -2 of tGradient[tCoordKey]) / 100 into tGradient[tCoordKey]
					end if
				end repeat

				/* Percentage values in the y coords are relative to the viewport
				 * height. */
				repeat for each item tCoordKey in "y1,y2,cy,fy"
					if tGradient[tCoordKey] ends with "%" then
						if xContext["root-height"] < 0 then
							throw "viewport relative grad coords not supported"
						end if
						put xContext["root-height"] * (char 1 to -2 of tGradient[tCoordKey]) / 100 into tGradient[tCoordKey]
					end if
				end repeat

				/* Percentage values in the r coords are relative to the viewport
				 * normalize diagonal (sqrt(w^2+h^2)/sqrt(2)). */
				repeat for each item tCoordKey in "r,fr"
					if tGradient[tCoordKey] ends with "%" then
						if xContext["root-height"] < 0 or \
							xContext["root-width"] < 0 then
							throw "viewport relative grad coords not supported"
						end if
						local tRootDiagonal
						put sqrt(xContext["root-width"]^2 + xContext["root-height"]^2) / sqrt(2) into tRootDiagonal
						put tRootDiagonal * (char 1 to -2 of tGradient[tCoordKey]) / 100 into tGradient[tCoordKey]
					end if
				end repeat
			end if
			seqAppend rCompiledPaint[3], tGradient["gradientTransform"]
			local tVectorMatrix
			put "matrix" into tVectorMatrix[1]
			if tServerElement["type"] is "linearGradient" then
				put tGradient["x2"] - tGradient["x1"] into tVectorMatrix[2]
				put tGradient["y2"] - tGradient["y1"] into tVectorMatrix[3]
				put tGradient["y2"] - tGradient["y1"] into tVectorMatrix[4]
				put tGradient["x1"] - tGradient["x2"] into tVectorMatrix[5]
				put tGradient["x1"] into tVectorMatrix[6]
				put tGradient["y1"] into tVectorMatrix[7]
			else
				put tGradient["r"] into tVectorMatrix[2]
				put 0 into tVectorMatrix[3]
				put 0 into tVectorMatrix[4]
				put tGradient["r"] into tVectorMatrix[5]
				put tGradient["cx"] into tVectorMatrix[6]
				put tGradient["cy"] into tVectorMatrix[7]
			end if
			seqPushOntoBack rCompiledPaint[3], tVectorMatrix
			put tGradient["colors"] into rCompiledPaint[4]
			put tGradient["offsets"] into rCompiledPaint[5]

			/* If the focal point matches the center point and the focal
			 * radius is 0 then this is a 'radial' gradient. Otherwise it is a
			 * two-point-conical gradient and the focal point/center must
			 * be mapped into the coordinate system created by the center
			 * point and radius. */
			if tGradient["fr"] is not 0 or \
				tGradient["fx"] is not tGradient["cx"] or \
				tGradient["fy"] is not tGradient["cy"] then
				put "conical" into rCompiledPaint[1]
				put (tGradient["fx"] - tGradient["cx"]) / tGradient["r"] into rCompiledPaint[6]
				put (tGradient["fy"] - tGradient["cy"]) / tGradient["r"] into rCompiledPaint[7]
				put tGradient["fr"] / tGradient["r"] into rCompiledPaint[8]
			end if
			return empty
		default
			break
		end switch
	end if
	_svgCompilePaint xContext, pFallback, rCompiledPaint
end _svgCompilePaintServer

private command _svgCompileResolveGradient @xContext, pElement, @rGradient
	local tBaseElement
	get pElement["features"]["xlink:href"]
	if it is empty then
		get pElement["features"]["href"]
	end if
	if it is not empty and \
		_svgContextLookup(xContext, it, tBaseElement) then
		_svgCompileResolveGradient xContext, tBaseElement, rGradient
	else
		put 0 into rGradient["x1"]
		put 0 into rGradient["y1"]
		put "100%" into rGradient["x2"]
		put 0 into rGradient["y2"]
		put "50%" into rGradient["cx"]
		put "50%" into rGradient["cy"]
		put "50%" into rGradient["r"]
		put "50%" into rGradient["fx"]
		put "50%" into rGradient["fy"]
		put 0 into rGradient["fr"]
		put "objectBoundingBox" into rGradient["gradientUnits"]
		put empty into rGradient["gradientTransform"]
		put "pad" into rGradient["spreadMethod"]
		put empty into rGradient["colors"]
		put empty into rGradient["offsets"]
	end if

	repeat for each item tProp in "x1,y1,x2,y2,cx,cy,r,fx,fy,fr,gradientUnits,gradientTransform,spreadMethod"
		if tProp is among the keys of pElement["features"] then
			put pElement["features"][tProp] into rGradient[tProp]
		end if
	end repeat

	if pElement["content"] is an array then
		local tColors, tOffsets
		repeat for each element tStop in pElement["content"]
			if tStop["type"] is "stop" then
				seqPushOntoBack tColors, tStop["features"]["stop-color"]["red"]
				seqPushOntoBack tColors, tStop["features"]["stop-color"]["green"]
				seqPushOntoBack tColors, tStop["features"]["stop-color"]["blue"]
				seqPushOntoBack tColors, tStop["features"]["stop-color"]["alpha"] * tStop["features"]["stop-opacity"]
				seqPushOntoBack tOffsets, tStop["features"]["offset"]
			end if
		end repeat
		if the number of elements in tColors > 0 then
			put tColors into rGradient["colors"]
			put tOffsets into rGradient["offsets"]
		end if
	end if
end _svgCompileResolveGradient

private command _svgCompileSvg @xContext, pElement
	local tX, tY, tWidth, tHeight, tViewBox, tPreserveAspectRatio
	put pElement["features"]["x"] into tX
	put pElement["features"]["y"] into tY
	put pElement["features"]["width"] into tWidth
	put pElement["features"]["height"] into tHeight
	put pElement["features"]["viewBox"] into tViewBox
	put pElement["features"]["preserveAspectRatio"] into tPreserveAspectRatio

	_svgContextSave xContext

	/* The SVG element establishes a new viewport so we must compute that
	 * transform here. */
	local tOldWidth, tOldHeight
	put xContext["root-width"] into tOldWidth
	put xContext["root-height"] into tOldHeight

	/* If the new width/height are relative, resolve them. */
	if tWidth < 0 then
		put tWidth * tOldWidth into tWidth
	end if
	if tHeight < 0 then
		put tHeight * tOldHeight into tHeight
	end if

	/* If there is a viewBox, then we must compute the viewBox transform. */
	if tViewBox is an array then
		local tVScaleX, tVScaleY, tVTranslateX, tVTranslateY

		put tWidth / tViewBox[3] into tVScaleX
		put tHeight / tViewBox[4] into tVScaleY

		if word 2 of tPreserveAspectRatio is "meet" then
			put min(tVScaleX, tVScaleY) into tVScaleX
			put tVScaleX into tVScaleY
		else if word 2 of tPreserveAspectRatio is "slice" then
			put max(tVScaleX, tVScaleY) into tVScaleX
			put tVScaleX into tVScaleY
		end if

		put tX - (tViewBox[1] * tVScaleX) into tVTranslateX
		put tY - (tViewBox[2] * tVScaleY) into tVTranslateY

		if tPreserveAspectRatio contains "xMid" then
			add (tWidth - tViewBox[3] * tVScaleX) / 2 to tVTranslateX
		else if tPreserveAspectRatio contains "xMax" then
			add (tWidth - tViewBox[3] * tVScaleX) to tVTranslateX
		end if

		if tPreserveAspectRatio contains "yMid" then
			add (tHeight - tViewBox[4] * tVScaleY) / 2 to tVTranslateY
		else if tPreserveAspectRatio contains "yMax" then
			add (tHeight - tViewBox[4] * tVScaleY) to tVTranslateY
		end if

		local tViewportMatrix
		put _svgTransformTranslate(tVTranslateX, tVTranslateY) into tViewportMatrix
		put _svgTransformConcat(tViewportMatrix, _svgTransformScale(tVScaleX, tVScaleY)) into tViewportMatrix

		local tTransform
		put _svgContextGetState(xContext, "transform") into tTransform
		seqPushOntoBack tTransform, _svgTransformFlatten(tViewportMatrix)
		_svgCompileTransform xContext, tTransform, empty, tTransform
		_svgContextSetState xContext, "transform", tTransform
	end if

	/* Update the root width/height */
	put tWidth into xContext["root-width"]
	put tHeight into xContext["root-height"]

	repeat for each element tChildElement in pElement["content"]
		_svgCompileElement xContext, tChildElement
	end repeat

	/* Restore the old root width/height */
	put tOldWidth into xContext["root-width"]
	put tOldHeight into xContext["root-height"]

	_svgContextRestore xContext
end _svgCompileSvg

private command _svgCompileRectangle @xContext, pElement
	local tX, tY, tWidth, tHeight, tRx, tRy
	put pElement["features"]["x"] into tX
	put pElement["features"]["y"] into tY
	put pElement["features"]["width"] into tWidth
	put pElement["features"]["height"] into tHeight
	put pElement["features"]["rx"] into tRx
	put pElement["features"]["ry"] into tRy
	if tRx is empty then
		put tRy into tRx
	else if tRy is empty then
		put tRx into tRy
	end if

	if tRx is empty then
		put 0 into tRx
	end if

	if tRy is empty then
		put 0 into tRy
	end if

	_svgCompileShape xContext, "rect", _svgBoxRectangle(tX, tY, tWidth, tHeight), tX, tY, tWidth, tHeight, tRx, tRy
end _svgCompileRectangle

private command _svgCompileCircle @xContext, pElement
	local tCx, tCy, tR
	put pElement["features"]["cx"] into tCx
	put pElement["features"]["cy"] into tCy
	put pElement["features"]["r"] into tR
	_svgCompileShape xContext, "circle", _svgBoxEllipse(tCx, tCy, tR, tR), tCx, tCy, tR
end _svgCompileCircle

private command _svgCompileEllipse @xContext, pElement
	local tCx, tCy, tRx, tRy
	put pElement["features"]["cx"] into tCx
	put pElement["features"]["cy"] into tCy
	put pElement["features"]["rx"] into tRx
	put pElement["features"]["ry"] into tRy
	_svgCompileShape xContext, "ellipse", _svgBoxEllipse(tCx, tCy, tRx, tRy), tCx, tCy, tRx, tRy
end _svgCompileEllipse

private command _svgCompileLine @xContext, pElement
	local tX1, tY1, tX2, tY2
	put pElement["features"]["x1"] into tX1
	put pElement["features"]["y1"] into tY1
	put pElement["features"]["x2"] into tX2
	put pElement["features"]["y2"] into tY2
	_svgCompileShape xContext, "line", _svgBoxLine(tX1, tY1, tX2, tY2), tX1, tY1, tX2, tY2
end _svgCompileLine

private command _svgCompilePolyline @xContext, pElement
	local tPoints
	put pElement["features"]["points"] into tPoints
	_svgCompileShape xContext, "polyline", _svgBoxPoints(tPoints), tPoints
end _svgCompilePolyline

private command _svgCompilePolygon @xContext, pElement
	local tPoints
	put pElement["features"]["points"] into tPoints
	_svgCompileShape xContext, "polygon", _svgBoxPoints(tPoints), tPoints
end _svgCompilePolygon

private command _svgCompilePath @xContext, pElement
	local tD
	put pElement["features"]["d"] into tD

	_svgCompileShape xContext, "path", _svgBoxPath(tD), tD["array"]
end _svgCompilePath

private command _svgCompileShape @xContext, pType, pBBox
	local tOperation
	put pType into tOperation[1]
	if param(4) is an array then
		put param(4) into tOperation[2]
	else
		repeat with tIndex = 4 to the paramCount
			put param(tIndex) into tOperation[2][tIndex - 3]
		end repeat
	end if

	_svgCompileState xContext, pBBox

	_svgContextEmit xContext, tOperation
end _svgCompileShape

private command _svgCompileState @xContext, pBBox
	local tCurrentState, tLastState
	put _svgContextGetCurrentState(xContext) into tCurrentState
	put _svgContextGetLastState(xContext) into tLastState

	/* This can be optimized to not emit fill/stroke properties if the fill
	 * or stroke is none, or the opacity is 0. */

	repeat for each key tStateName in tCurrentState
		/* If the state is fill or stroke, we must finish compiling it as it
		 * might contain transforms relative to the element. */
		if tStateName is "fill" or tStateName is "stroke" then
			_svgCompilePartialPaint xContext, tCurrentState[tStateName], pBBox
		end if

		if tCurrentState[tStateName] is tLastState[tStateName] then
			next repeat
		end if

		local tOperation
		put empty into tOperation

		put tStateName into tOperation[1]
		seqPushOntoBack tOperation, tCurrentState[tStateName]

		_svgContextEmit xContext, tOperation
	end repeat
	_svgContextSetLastState xContext, tCurrentState
end _svgCompileState

/*******************************************************************************
 *
 *  SVG BBOX OPERATIONS
 *
 ******************************************************************************/

private function _svgBoxMake pLeft, pTop, pRight, pBottom
	local tBox
	put pLeft into tBox["left"]
	put pTop into tBox["top"]
	put pRight into tBox["right"]
	put pBottom into tBox["bottom"]
	return tBox
end _svgBoxMake

private function _svgBoxRectangle pX, pY, pWidth, pHeight
	return _svgBoxMake(pX, pY, pX + pWidth, pY + pHeight)
end _svgBoxRectangle

private function _svgBoxEllipse pCx, pCy, pRx, pRy
	return _svgBoxMake(pCx - pRx, pCy - pRy, pCx + pRx, pCy + pRy)
end _svgBoxEllipse

private function _svgBoxLine pX1, pY1, pX2, pY2
	return _svgBoxMake(min(pX1, pX2), min(pY1, pY2), max(pX1, pX2), max(pY1, pY2))
end _svgBoxLine

private function _svgBoxPoints pPoints
	local tLeft, tTop, tRight, tBottom
	put pPoints[1] into tLeft
	put pPoints[1] into tRight
	put pPoints[2] into tTop
	put pPoints[2] into tBottom
   local tNumberPoints
   put the number of elements in pPoints into tNumberPoints
	repeat with i = 1 to tNumberPoints step 2
      if pPoints[i] < tLeft then
			put pPoints[i] into tLeft
		else if pPoints[i] > tRight then
			put pPoints[i] into tRight
		end if
		if pPoints[i + 1] < tTop then
			put pPoints[i + 1] into tTop
		else if pPoints[i + 1] > tBottom then
			put pPoints[i + 1] into tBottom
		end if
	end repeat
	return _svgBoxMake(tLeft, tTop, tRight, tBottom)
end _svgBoxPoints

private function _svgBoxPath pPath
	_internal vectorpath getbbox pPath["string"]

	local tBBox
	put item 1 of the result + 0 into tBBox["left"]
	put item 2 of the result + 0 into tBBox["top"]
	put item 3 of the result + 0 into tBBox["right"]
	put item 4 of the result + 0 into tBBox["bottom"]
	return tBBox
end _svgBoxPath

/*******************************************************************************
 *
 *  SVG ENCODE OPERATIONS
 *
 ******************************************************************************/

/* The kMCGDrawing constants are taken from those defined in the
 * libgraphics/drawing.cpp file and must always match. */

constant kMCGDrawingIdent = "LCD"
constant kMCGDrawingVersion = 0

constant kMCGDrawingOpcodeEnd = 0
constant kMCGDrawingOpcodeTransform = 1
constant kMCGDrawingOpcodeFillPaint = 2
constant kMCGDrawingOpcodeFillOpacity = 3
constant kMCGDrawingOpcodeFillRule = 4
constant kMCGDrawingOpcodeStrokePaint = 5
constant kMCGDrawingOpcodeStrokeOpacity = 6
constant kMCGDrawingOpcodeStrokeWidth = 7
constant kMCGDrawingOpcodeStrokeLineJoin = 8
constant kMCGDrawingOpcodeStrokeLineCap = 9
constant kMCGDrawingOpcodeStrokeDashArray = 10
constant kMCGDrawingOpcodeStrokeDashOffset = 11
constant kMCGDrawingOpcodeStrokeMiterLimit = 12
constant kMCGDrawingOpcodeRectangle = 13
constant kMCGDrawingOpcodeRoundedRectangle = 14
constant kMCGDrawingOpcodeCircle = 15
constant kMCGDrawingOpcodeEllipse = 16
constant kMCGDrawingOpcodeLine = 17
constant kMCGDrawingOpcodePolyline = 18
constant kMCGDrawingOpcodePolygon = 19
constant kMCGDrawingOpcodePath = 20

constant kMCGDrawingTransformOpcodeIdentity = 0
constant kMCGDrawingTransformOpcodeAffine = 1

constant kMCGDrawingPaintOpcodeNone = 0
constant kMCGDrawingPaintOpcodeSolidColor = 1
constant kMCGDrawingPaintOpcodeLinearGradient = 2
constant kMCGDrawingPaintOpcodeRadialGradient = 3
constant kMCGDrawingPaintOpcodeConicalGradient = 4
constant kMCGDrawingPaintOpcodeCurrentColor = 5

constant kMCGDrawingSpreadMethodOpcodePad = 0
constant kMCGDrawingSpreadMethodOpcodeReflect = 1
constant kMCGDrawingSpreadMethodOpcodeRepeat = 2

constant kMCGDrawingFillRuleOpcodeNonZero = 0
constant kMCGDrawingFillRuleOpcodeEvenOdd = 1

constant kMCGDrawingStrokeLineJoinOpcodeBevel = 0
constant kMCGDrawingStrokeLineJoinOpcodeRound = 1
constant kMCGDrawingStrokeLineJoinOpcodeMiter = 2

constant kMCGDrawingStrokeLineCapOpcodeButt = 0
constant kMCGDrawingStrokeLineCapOpcodeRound = 1
constant kMCGDrawingStrokeLineCapOpcodeSquare = 2

constant kMCGDrawingPathOpcodeEnd = 0
constant kMCGDrawingPathOpcodeMoveTo = 1
constant kMCGDrawingPathOpcodeLineTo = 2
constant kMCGDrawingPathOpcodeHorizontalTo = 3
constant kMCGDrawingPathOpcodeVerticalTo = 4
constant kMCGDrawingPathOpcodeCubicTo = 5
constant kMCGDrawingPathOpcodeSmoothCubicTo = 6
constant kMCGDrawingPathOpcodeQuadraticTo= 7
constant kMCGDrawingPathOpcodeSmoothQuadraticTo = 8
constant kMCGDrawingPathOpcodeArcTo = 9
constant kMCGDrawingPathOpcodeReflexArcTo = 10
constant kMCGDrawingPathOpcodeReverseArcTo = 11
constant kMCGDrawingPathOpcodeReverseReflexArcTo = 12
constant kMCGDrawingPathOpcodeCloseSubpath = 13

private command _svgEncode @xContext, @rDrawing
	put empty into xContext["opcodes"]
	put empty into xContext["scalars"]
	repeat for each element tOperation in xContext["operations"]
		local tCommand, tArgument
		put tOperation[1] into tCommand
		put tOperation[2] into tArgument
		switch tCommand
		case "transform"
			_svgEncodeOp xContext, kMCGDrawingOpcodeTransform
			_svgEncodeTransform xContext, tArgument
			break
		case "fill"
			_svgEncodeOp xContext, kMCGDrawingOpcodeFillPaint
			_svgEncodePaint xContext, tArgument
			break
		case "fill-opacity"
			_svgEncodeOp xContext, kMCGDrawingOpcodeFillOpacity, tArgument
			break
		case "fill-rule"
			_svgEncodeOp xContext, kMCGDrawingOpcodeFillRule
			if tArgument is "nonzero" then
				_svgEncodeOp xContext, kMCGDrawingFillRuleOpcodeNonZero
			else if tArgument is "evenodd" then
				_svgEncodeOp xContext, kMCGDrawingFillRuleOpcodeEvenOdd
			else
				_InternalError format("unknown fill-rule '%s'", tArgument)
			end if
			break
		case "stroke"
			_svgEncodeOp xContext, kMCGDrawingOpcodeStrokePaint
			_svgEncodePaint xContext, tArgument
			break
		case "stroke-opacity"
			_svgEncodeOp xContext, kMCGDrawingOpcodeStrokeOpacity, tArgument
			break
		case "stroke-width"
			_svgEncodeOp xContext, kMCGDrawingOpcodeStrokeWidth, tArgument
			break
		case "stroke-linejoin"
			_svgEncodeOp xContext, kMCGDrawingOpcodeStrokeLineJoin
			switch tArgument
			case "bevel"
				_svgEncodeOp xContext, kMCGDrawingStrokeLineJoinOpcodeBevel
				break
			case "round"
				_svgEncodeOp xContext, kMCGDrawingStrokeLineJoinOpcodeRound
				break
			case "miter"
				_svgEncodeOp xContext, kMCGDrawingStrokeLineJoinOpcodeMiter
				break
			end switch
			break
		case "stroke-linecap"
			_svgEncodeOp xContext, kMCGDrawingOpcodeStrokeLineCap
			switch tArgument
			case "butt"
				_svgEncodeOp xContext, kMCGDrawingStrokeLineCapOpcodeButt
				break
			case "round"
				_svgEncodeOp xContext, kMCGDrawingStrokeLineCapOpcodeRound
				break
			case "square"
				_svgEncodeOp xContext, kMCGDrawingStrokeLineCapOpcodeSquare
				break
			end switch
			break
		case "stroke-dasharray"
			_svgEncodeOp xContext, kMCGDrawingOpcodeStrokeDashArray, tArgument
			break
		case "stroke-dashoffset"
			_svgEncodeOp xContext, kMCGDrawingOpcodeStrokeDashOffset, tArgument
			break
		case "stroke-miterlimit"
			_svgEncodeOp xContext, kMCGDrawingOpcodeStrokeMiterLimit, tArgument
			break
		case "rect"
			if tArgument[5] is 0 and tArgument[6] is 0 then
				_svgEncodeOp xContext, kMCGDrawingOpcodeRectangle, tArgument[1], tArgument[2], tArgument[3], tArgument[4]
			else
				_svgEncodeOp xContext, kMCGDrawingOpcodeRoundedRectangle, tArgument[1], tArgument[2], tArgument[3], tArgument[4], tArgument[5], tArgument[6]
			end if
			break
		case "circle"
			_svgEncodeOp xContext, kMCGDrawingOpcodeCircle, tArgument[1], tArgument[2], tArgument[3]
			break
		case "ellipse"
			_svgEncodeOp xContext, kMCGDrawingOpcodeEllipse, tArgument[1], tArgument[2], tArgument[3], tArgument[4]
			break
		case "line"
			_svgEncodeOp xContext, kMCGDrawingOpcodeLine, tArgument[1], tArgument[2], tArgument[3], tArgument[4]
			break
		case "polyline"
			_svgEncodeOp xContext, kMCGDrawingOpcodePolyline, tArgument
			break
		case "polygon"
			_svgEncodeOp xContext, kMCGDrawingOpcodePolygon, tArgument
			break
		case "path"
			_svgEncodeOp xContext, kMCGDrawingOpcodePath
			_svgEncodePath xContext, tArgument
			break
		default
			_InternalError format("unknown command '%s'", tOperation[1])
			break
		end switch
	end repeat

	_svgEncodeOp xContext, kMCGDrawingOpcodeEnd

	local tDrawing
	put binaryEncode("A3C", kMCGDrawingIdent, kMCGDrawingVersion) into tDrawing
	put binaryEncode("ff", xContext["drawing-width"], xContext["drawing-height"]) after tDrawing
	put binaryEncode("I", (the number of bytes in xContext["scalars"]) div 4) after tDrawing
	put xContext["scalars"] after tDrawing
	put binaryEncode("I", the number of bytes in xContext["opcodes"]) after tDrawing
	put xContext["opcodes"] after tDrawing
	put tDrawing into rDrawing
end _svgEncode

private function _svgEncodeIndex pIndex
	local tValue
	put empty into tValue
	repeat while pIndex >= 128
		put numToByte(128 + (pIndex mod 128)) after tValue
		put pIndex div 128 into pIndex
	end repeat
	put numToByte(pIndex mod 128) after tValue
	return tValue
end _svgEncodeIndex

private command _svgEncodeOp @xContext, pOpcode
	local tParams
	repeat with i = 3 to the paramCount
		put param(i) into tParams[i - 2]
	end repeat
	_svgEncodeOpV xContext, pOpcode, tParams
end _svgEncodeOp

private command _svgEncodeOpV @xContext, pOpcode, pArguments
	put numToByte(pOpcode) after xContext["opcodes"]
	repeat for each element tArgument in pArguments
		if tArgument is an array or tArgument is empty then
			put _svgEncodeIndex(the number of elements in tArgument) after xContext["opcodes"]
			repeat for each element tElement in tArgument
				if tElement is not a number then
					_InternalError format("invalid scalar value '%s' in list", tArgument)
				end if
				put binaryEncode("f", tElement) after xContext["scalars"]
			end repeat
		else if tArgument is a number then
			put binaryEncode("f", tArgument) after xContext["scalars"]
		else
			_InternalError format("invalid scalar value '%s'", tArgument)
		end if
	end repeat
end _svgEncodeOpV

private command _svgEncodeIndex @xContext, pIndex
	put _svgEncodeIndex(pIndex) after xContext["opcodes"]
end _svgEncodeIndex

private command _svgEncodeScalars @xContext, pScalars
	if pScalars is an array then
		repeat for each element tScalar in pScalars
			put binaryEncode("f", tScalar) after xContext["scalars"]
		end repeat
	else
		repeat with i = 2 to the paramCount
			put binaryEncode("f", param(i)) after xContext["scalars"]
		end repeat
	end if
end _svgEncodeScalars

private command _svgEncodePaint @xContext, pPaint
	if pPaint[1] is "none" then
		_svgEncodeOp xContext, kMCGDrawingPaintOpcodeNone
	else if pPaint[1] is "current" then
		_svgEncodeOp xContext, kMCGDrawingPaintOpcodeCurrentColor
	else if pPaint[1] is "color" then
		_svgEncodeOp xContext, kMCGDrawingPaintOpcodeSolidColor, pPaint[2], pPaint[3], pPaint[4], pPaint[5]
	else if pPaint[1] is "linear" or pPaint[1] is "radial" or pPaint[1] is "conical" then
		if pPaint[1] is "linear" then
			_svgEncodeOp xContext, kMCGDrawingPaintOpcodeLinearGradient
		else if pPaint[1] is "radial" then
			_svgEncodeOp xContext, kMCGDrawingPaintOpcodeRadialGradient
		else if pPaint[1] is "conical" then
			_svgEncodeOp xContext, kMCGDrawingPaintOpcodeConicalGradient
		end if
		switch pPaint[2]
		case "pad"
			_svgEncodeOp xContext, kMCGDrawingSpreadMethodOpcodePad
			break
		case "reflect"
			_svgEncodeOp xContext, kMCGDrawingSpreadMethodOpcodeReflect
			break
		case "repeat"
			_svgEncodeOp xContext, kMCGDrawingSpreadMethodOpcodeRepeat
			break
		end switch
		_svgEncodeTransform xContext, pPaint[3]
		_svgEncodeIndex xContext, the number of elements in pPaint[5]
		_svgEncodeScalars xContext, pPaint[4]
		_svgEncodeScalars xContext, pPaint[5]
		if pPaint[1] is "conical" then
			_svgEncodeScalars xContext, pPaint[6], pPaint[7], pPaint[8]
		end if
	else
		_InternalError format("unknown paint '%s'", pPaint[1])
	end if
end _svgEncodePaint

private command _svgEncodeTransform @xContext, pTransform
	if the number of elements in pTransform > 1 then
		throw "internal error - transform list length is not 1"
	end if

	if the number of elements in pTransform is 0 then
		_svgEncodeOp xContext, kMCGDrawingTransformOpcodeIdentity
	else
		local tTransform
		put pTransform[1] into tTransform
		switch tTransform[1]
		case "matrix"
			_svgEncodeOp xContext, kMCGDrawingTransformOpcodeAffine, tTransform[2], tTransform[3], tTransform[4], tTransform[5], tTransform[6], tTransform[7]
			break
		default
			_InternalError format("unknown transform '%s'", tTransform[1])
		end switch
	end if
end _svgEncodeTransform

private command _svgEncodePath @xContext, pPath
	local tFirstX, tFirstY
	put 0 into tFirstX
	put 0 into tFirstY

	local tLastX, tLastY
	put 0 into tLastX
	put 0 into tLastY

	local tScalarIndex
	set the caseSensitive to true
	put 1 into tScalarIndex
	repeat for each char tCommand in pPath["commands"]
		local tScalars
		put pPath["scalars"][tScalarIndex] into tScalars
		add 1 to tScalarIndex

		local tOpcode
		switch tCommand
		case "m"
			add tLastX to tScalars[1]
			add tLastY to tScalars[2]
		case "M"
			put kMCGDrawingPathOpcodeMoveTo into tOpcode
			put tScalars[1] into tLastX
			put tScalars[2] into tLastY
			put tLastX into tFirstX
			put tLastY into tFirstY
			break

		case "Z"
		case "z"
			put kMCGDrawingPathOpcodeCloseSubpath into tOpcode
			put tFirstX into tLastX
			put tFirstY into tLastY
			break

		case "l"
			add tLastX to tScalars[1]
			add tLastY to tScalars[2]
		case "L"
			put kMCGDrawingPathOpcodeLineTo into tOpcode
			put tScalars[1] into tLastX
			put tScalars[2] into tLastY
			break

		case "h"
			add tLastX to tScalars[1]
		case "H"
			put kMCGDrawingPathOpcodeHorizontalTo into tOpcode
			put tScalars[1] into tLastX
			break

		case "v"
			add tLastY to tScalars[1]
		case "V"
			put kMCGDrawingPathOpcodeVerticalTo into tOpcode
			put tScalars[1] into tLastY
			break

		case "c"
			add tLastX to tScalars[1]
			add tLastY to tScalars[2]
			add tLastX to tScalars[3]
			add tLastY to tScalars[4]
			add tLastX to tScalars[5]
			add tLastY to tScalars[6]
		case "C"
			put kMCGDrawingPathOpcodeCubicTo into tOpcode
			put tScalars[5] into tLastX
			put tScalars[6] into tLastY
			break

		case "s"
			add tLastX to tScalars[1]
			add tLastY to tScalars[2]
			add tLastX to tScalars[3]
			add tLastY to tScalars[4]
		case "S"
			put kMCGDrawingPathOpcodeSmoothCubicTo into tOpcode
			put tScalars[3] into tLastX
			put tScalars[4] into tLastY
			break

		case "q"
			add tLastX to tScalars[1]
			add tLastY to tScalars[2]
			add tLastX to tScalars[3]
			add tLastY to tScalars[4]
		case "Q"
			put kMCGDrawingPathOpcodeQuadraticTo into tOpcode
			put tScalars[3] into tLastX
			put tScalars[4] into tLastY
			break

		case "t"
			add tLastX to tScalars[1]
			add tLastY to tScalars[2]
		case "T"
			put kMCGDrawingPathOpcodeSmoothQuadraticTo into tOpcode
			put tScalars[1] into tLastX
			put tScalars[2] into tLastY
			break

		case "a"
			add tLastX to tScalars[6]
			add tLastY to tScalars[7]
		case "A"
			if tScalars[4] is "0" then
				if tScalars[5] is "0" then
					put kMCGDrawingPathOpcodeArcTo into tOpcode
				else
					put kMCGDrawingPathOpcodeReverseArcTo into tOpcode
				end if
			else
				if tScalars[5] is "0" then
					put kMCGDrawingPathOpcodeReflexArcTo into tOpcode
				else
					put kMCGDrawingPathOpcodeReverseReflexArcTo into tOpcode
				end if
			end if
			put tScalars[6] into tScalars[4]
			put tScalars[7] into tScalars[5]
			delete variable tScalars[6]
			delete variable tScalars[7]
			put tScalars[4] into tLastX
			put tScalars[5] into tLastY
			break
		end switch
		_svgEncodeOpV xContext, tOpcode, tScalars
	end repeat
	_svgEncodeOp xContext, kMCGDrawingPathOpcodeEnd
end _svgEncodePath

/*******************************************************************************
 *
 *  SVG TRANSFORM OPERATIONS
 *
 ******************************************************************************/

private function _svgTransformIsIdentity pT
	return pT is _svgTransformIdentity()
end _svgTransformIsIdentity

private function _svgTransformMatrix pA, pB, pC, pD, pE, pF
	local tR
	put pA into tR[1,1]
	put pC into tR[1,2]
	put pE into tR[1,3]
	put pB into tR[2,1]
	put pD into tR[2,2]
	put pF into tR[2,3]
	put 0 into tR[3,1]
	put 0 into tR[3,2]
	put 1 into tR[3,3]
	return tR
end _svgTransformMatrix

private function _svgTransformIdentity
	return _svgTransformMatrix(1, 0, 0, 1, 0, 0)
end _svgTransformIdentity

private function _svgTransformTranslate pTx, pTy
	return _svgTransformMatrix(1, 0, 0, 1, pTx, pTy)
end _svgTransformTranslate

private function _svgTransformScale pSx, pSy
	return _svgTransformMatrix(pSx, 0, 0, pSy, 0, 0)
end _svgTransformScale

private function _svgTransformRotate pAngle
	local tRadians
	put pAngle * pi / 180 into tRadians
	return _svgTransformMatrix(cos(tRadians), sin(tRadians), -sin(tRadians), cos(tRadians), 0, 0)
end _svgTransformRotate

private function _svgTransformSkew pXa, pYa
	return _svgTransformMatrix(1, tan(pYa * pi / 180), tan(pXa * pi / 180), 1, 0, 0)
end _svgTransformSkew

private function _svgTransformConcat pLeft, pRight
	return matrixMultiply(pLeft, pRight)
end _svgTransformConcat

private function _svgTransformUnflatten pFlatMatrix
	return _svgTransformMatrix(pFlatMatrix[2], pFlatMatrix[3], pFlatMatrix[4], pFlatMatrix[5], pFlatMatrix[6], pFlatMatrix[7])
end _svgTransformUnflatten

private function _svgTransformFlatten pMatrix
	local tS
	put "matrix" into tS[1]
	put pMatrix[1,1] into tS[2]
	put pMatrix[2,1] into tS[3]
	put pMatrix[1,2] into tS[4]
	put pMatrix[2,2] into tS[5]
	put pMatrix[1,3] into tS[6]
	put pMatrix[2,3] into tS[7]
	return tS
end _svgTransformFlatten

/*******************************************************************************
 *
 *  SVG VALUE PARSING OPERATIONS
 *
 ******************************************************************************/

private function _svgParseFeatureValue @xContext, pElementType, pFeatureName, @xValue
	/* The syntax of a value is represented as a sequence of possible values
	 * or patterns. */
	local tType
	put svgSpecGetFeatureTypeForElement(pFeatureName, pElementType) into tType

	repeat for each element tVariant in tType
		if not (tVariant begins with "<") then
			if xValue is tVariant then
				return true
			end if
		else
			switch tVariant
			case "<identifier>"
				if _svgParseIdentifierValue(xValue) then
					return true
				end if
				break
			case "<boolean>"
				if _svgParseBooleanValue(xValue) then
					return true
				end if
				break
			case "<text>"
				if _svgParseTextValue(xValue) then
					return true
				end if
				break
			case "<transform>"
				if _svgParseTransformValue(xValue) then
					return true
				end if
				break
			case "<paint>"
				if _svgParsePaintValue(xValue) then
					return true
				end if
				break
			case "<color>"
				if _svgParseColorValue(xValue) then
					return true
				end if
				break
			case "<coordinate>"
			case "<length>"
				if _svgParseLengthValue(xValue) then
					return true
				end if
				break
			case "<size>"
				if _svgParseSizeValue(xValue) then
					return true
				end if
				break
			case "<nonnegative-length>"
				if _svgParseNonNegativeLengthValue(xValue) then
					return true
				end if
				break
			case "<number>"
				if _svgParseNumberValue(xValue) then
					return true
				end if
				break
			case "<path>"
				if _svgParsePathValue(xValue) then
					return true
				end if
				break
			case "<rectangle>"
				if _svgParseRectangleValue(xValue) then
					return true
				end if
				break
			case "<normalized-rectangle>"
				if _svgParseNormalizedRectangleValue(xValue) then
					return true
				end if
				break
			case "<reference>"
				if _svgParseReferenceValue(xValue) then
					return true
				end if
				break
			case "<miterlimit>"
				if _svgParseMiterLimitValue(xValue) then
					return true
				end if
				break
			case "<opacity>"
				if _svgParseOpacityValue(xValue) then
					return true
				end if
				break
			case "<dasharray>"
				if _svgParseDashArrayValue(xValue) then
					return true
				end if
				break
			case "<points>"
				if _svgParsePointsValue(xValue) then
					return true
				end if
				break
			case "<preserve-aspect-ratio>"
				if _svgParsePreserveAspectRatio(xValue) then
					return true
				end if
				break
			case "<reference>"
				if _svgParseReferenceValue(xValue) then
					return true
				end if
				break
			case "<percent-coordinate>"
				if _svgParsePercentCoordinateValue(xValue) then
					return true
				end if
				break
			case "<percent-length>"
				if _svgParsePercentLengthValue(xValue) then
					return true
				end if
			default
				_log format("unimplemented value pattern '%s'", tVariant)
				break
			end switch
		end if
	end repeat

	return false
end _svgParseFeatureValue

private function _svgParseStyleAttributeValue @xValue
	local tStyle
	split xValue by ";"
	repeat for each element tAttr in xValue
		split tAttr by ":"
		put word 1 to -1 of tAttr[2] into tStyle[word 1 to -1 of tAttr[1]]
	end repeat
	put tStyle into xValue
	return true
end _svgParseStyleAttributeValue

private function _svgParsePreserveAspectRatio @xValue
	if xValue is "none" then
		return true
	end if
	if word 1 of xValue is among the items of "xMinYMin,xMidYMin,xMaxYMin,xMinYMid,xMidYMid,xMaxYMid,xMinYMax,xMidYMax,xMaxYMax" then
		if word 2 of xValue is empty then
			put " meet" after xValue
		end if
		if word 2 of xValue is among the items of "slice,meet" then
			return true
		end if
	end if
	return false
end _svgParsePreserveAspectRatio

/* The <color> type represents a color:
 *
 *   color
 *     : #rgb
 *     | #rrggbb
 *     | rgb(rrr, ggg, bbb)
 *     | rgb(R%, G%, B%)
 *     | black | silver | gray | white | maroon | red | purple | fuchsia |
 *     | green | lime | olive | yellow | navy | blue | teal | aqua
 *
 */
private function _svgParseColorValue @xValue
	local tRed, tGreen, tBlue
	if matchText(xValue, "^\#[0-9a-zA-Z]{3}$") then
		put baseConvert(char 2 of xValue, 16, 10) / 15 into tRed
		put baseConvert(char 3 of xValue, 16, 10) / 15 into tGreen
		put baseConvert(char 4 of xValue, 16, 10) / 15 into tBlue
	else if matchText(xValue, "^\#[0-9a-zA-Z]{6}$") then
		put baseConvert(char 2 to 3 of xValue, 16, 10) / 255 into tRed
		put baseConvert(char 4 to 5 of xValue, 16, 10) / 255 into tGreen
		put baseConvert(char 6 to 7 of xValue, 16, 10) / 255 into tBlue
	else
		if not svgSpecGetColor(xValue, tRed, tGreen, tBlue) and \
			not matchText(xValue, "^rgb\s*\(\s*([0-9]+)\s*\,\s*([0-9]+)\s*,\s*([0-9]+)\s*\)$", tRed, tGreen, tBlue) then
			return false
		end if
		divide tRed by 255
		divide tGreen by 255
		divide tBlue by 255
	end if

	put "color" into xValue["type"]
	put tRed + 0 into xValue["red"]
	put tGreen + 0 into xValue["green"]
	put tBlue + 0 into xValue["blue"]
	put 1 into xValue["alpha"]

	return true
end _svgParseColorValue

private command _svgNormalizeLength @xValue, pUnit
	switch pUnit
	case "px"
		break
	case "pt"
		put xValue * 96 / 72 into xValue
		break
	case "pc"
		put xValue * 96 / 6 into xValue
		break
	case "in"
		put xValue * 96 into xValue
		break
	case "q"
		put xValue * 96 / (40 * 2.54) into xValue
		break
	case "mm"
		put xValue * 96 / 25.4 into xValue
		break
	case "cm"
		put xValue * 96 / 2.54 into xValue
		break
	end switch
end _svgNormalizeLength

private function _svgParseLengthValue @xValue
	local tValue, tValueUnit
	put word 1 to -1 of xValue into xValue
	repeat for each item tUnit in "in,cm,mm,pt,pc,px"
		if xValue ends with tUnit then
			delete char -(the length of tUnit) to -1 of xValue
			put tUnit into tValueUnit
			exit repeat
		end if
	end repeat
	if _svgParseNumberValue(xValue) then
		_svgNormalizeLength xValue, tValueUnit
		return true
	end  if
	return false
end _svgParseLengthValue

private function _svgParsePercentLengthValue @xValue
	if _svgParseLengthValue(xValue) then
		return true
	end if

	put word 1 to -1 of xValue into xValue
	if char -1 of xValue is "%" then
		delete the last char of xValue
		if _svgParseLengthValue(xValue) then
			put "%" after xValue
			return true
		end if
	end if
	return false
end _svgParsePercentLengthValue

private function _svgParsePercentCoordinateValue @xValue
	return _svgParsePercentLengthValue(xValue)
end _svgParsePercentCoordinateValue

private function _svgParseSizeValue @xValue
	local tValue, tValueUnit
	put word 1 to -1 of xValue into xValue
	repeat for each item tUnit in "in,cm,mm,pt,pc,px,%"
		if xValue ends with tUnit then
			delete char -(the length of tUnit) to -1 of xValue
			put tUnit into tValueUnit
			exit repeat
		end if
	end repeat
	if _svgParseNumberValue(xValue) then
		/* TODO: Handle unit in an appropriate way. */
		if tValueUnit is "%" then
			put -xValue / 100 into xValue
		else
			_svgNormalizeLength xValue, tValueUnit
		end if
		return true
	end  if
	return false
end _svgParseSizeValue

private function _svgParseNonNegativeLengthValue @xValue
	local tValue, tUnit
	if _svgParseLengthValue(xValue) then
		if xValue >= 0 then
			return true
		end if
	end if
	return false
end _svgParseNonNegativeLengthValue

private function _svgParseNumberValue @xValue
	if xValue is a number then
		put xValue + 0 into xValue
		return true
	end if
	return false
end _svgParseNumberValue

private function _svgParseNonNegativeNumberValue @xValue
	if _svgParseNumberValue(xValue) then
		if xValue >= 0 then
			return true
		end if
	end if
	return false
end _svgParseNonNegativeNumberValue

private function _svgParsePathValue @xValue
	local tPath
	put empty into tPath

	local tString
	put xValue into tString

	local tValid
	put _svgPathParseSpace(tPath, tString) into tValid
	repeat while tValid and tString is not empty
		put _svgPathParseCommand(tPath, tString) into tValid
	end repeat

	if not tValid then
		return false
	end if

	put xValue into xValue["string"]
	put tPath into xValue["array"]

	return true
end _svgParsePathValue

private function _svgPathParseSpace @xPath, @xString
	repeat while char 1 of xString is among the chars of (tab & space & return & numToChar(13))
		delete char 1 of xString
	end repeat
	return true
end _svgPathParseSpace

private function _svgPathParseCommaSpace @xPath, @xString
	if char 1 of xString is comma then
		delete char 1 of xString
		return _svgPathParseSpace(xPath, xString)
	end if

	if not _svgPathParseSpace(xPath, xString) then
		return false
	end if

	if char 1 of xString is comma then
		delete char 1 of xString
	end if

	return _svgPathParseSpace(xPath, xString)
end _svgPathParseCommaSpace

private function _svgPathParseCommand @xPath, @xString
	switch char 1 of xString
	case "z"
		return _svgPathParseEmptyCommand(xPath, xString)
	case "m"
	case "l"
	case "t"
		return _svgPathParseCoordinatePairCommand(xPath, xString, 1)
	case "h"
	case "v"
		return _svgPathParseCoordinateCommand(xPath, xString)
	case "s"
	case "q"
		return _svgPathParseCoordinatePairCommand(xPath, xString, 2)
	case "c"
		return _svgPathParseCoordinatePairCommand(xPath, xString, 3)
	case "a"
		return _svgPathParseArcCommand(xPath, xString)
	end switch
	return false
end _svgPathParseCommand

private function _svgPathParseEmptyCommand @xPath, @xString
	local tCommand
	put char 1 of xString into tCommand
	delete char 1 of xString

	put tCommand after xPath["commands"]
	seqPushOntoBack xPath["scalars"], empty

	return _svgPathParseSpace(xPath, xString)
end _svgPathParseEmptyCommand

private function _svgPathParseCoordinatePairCommand @xPath, @xString, pArity
	local tCommand
	put char 1 of xString into tCommand
	delete char 1 of xString

	local tValid
	put _svgPathParseSpace(xPath, xString) into tValid
	repeat while tValid and char 1 of xString is among the chars of "0123456789+-."
		put tCommand after xPath["commands"]
		seqPushOntoBack xPath["scalars"], empty
		repeat pArity times
			if tValid then put _svgPathParseCoordinatePair(xPath, xString) into tValid
			if tValid then put _svgPathParseCommaSpace(xPath, xString) into tValid
		end repeat

		set the caseSensitive to true
		if tCommand is "m" then
			put "l" into tCommand
		else if tCommand is "M" then
			put "L" into tCommand
		end if
		set the caseSensitive to false
	end repeat

	return tValid
end _svgPathParseCoordinatePairCommand

private function _svgPathParseCoordinateCommand @xPath, @xString
	local tCommand
	put char 1 of xString into tCommand
	delete char 1 of xString

	local tValid
	put _svgPathParseSpace(xPath, xString) into tValid
	repeat while tValid and char 1 of xString is among the chars of "0123456789+-."
		put tCommand after xPath["commands"]
		seqPushOntoBack xPath["scalars"], empty
		if tValid then put _svgPathParseCoordinate(xPath, xString) into tValid
		if tValid then put _svgPathParseCommaSpace(xPath, xString) into tValid
	end repeat

	return tValid
end _svgPathParseCoordinateCommand

private function _svgPathParseArcCommand @xPath, @xString
	local tCommand
	put char 1 of xString into tCommand
	delete char 1 of xString

	local tValid
	put _svgPathParseSpace(xPath, xString) into tValid
	repeat while tValid and char 1 of xString is among the chars of "0123456789+-."
		put tCommand after xPath["commands"]
		seqPushOntoBack xPath["scalars"], empty
		if tValid then put _svgPathParseCoordinatePair(xPath, xString) into tValid
		if tValid then put _svgPathParseCommaSpace(xPath, xString) into tValid
		if tValid then put _svgPathParseCoordinate(xPath, xString) into tValid
		if tValid then put _svgPathParseCommaSpace(xPath, xString) into tValid
		if tValid then put _svgPathParseFlag(xPath, xString) into tValid
		if tValid then put _svgPathParseCommaSpace(xPath, xString) into tValid
		if tValid then put _svgPathParseFlag(xPath, xString) into tValid
		if tValid then put _svgPathParseCommaSpace(xPath, xString) into tValid
		if tValid then put _svgPathParseCoordinatePair(xPath, xString) into tValid
		if tValid then put _svgPathParseCommaSpace(xPath, xString) into tValid
	end repeat

	return tValid
end _svgPathParseArcCommand

private function _svgPathParseCoordinatePair @xPath, @xString
	local tValid
	put _svgPathParseCoordinate(xPath, xString) into tValid
	if tValid then put _svgPathParseCommaSpace(xPath, xString) into tValid
	if tValid then put _svgPathParseCoordinate(xPath, xString) into tValid
	return tValid
end _svgPathParseCoordinatePair

private function _svgPathParseCoordinate @xPath, @xString
	return _svgPathParseNumber(xPath, xString)
end _svgPathParseCoordinate

private function _svgPathParseFlag @xPath, @xString
	if char 1 of xString is among the chars of "01" then
		seqPushOntoBack xPath["scalars"][the number of elements in xPath["scalars"]], char 1 of xString
		delete char 1 of xString
		return true
	end if
	return false
end _svgPathParseFlag

private function _svgPathParseNumber @xPath, @xString
	local tNumber
	if char 1 of xString is among the chars of "+-" then
		put char 1 of xString after tNumber
		delete char 1 of xString
	end if
	repeat while char 1 of xString is among the chars of "0123456789"
		put char 1 of xString after tNumber
		delete char 1 of xString
	end repeat
	if char 1 of xString is "." then
		put char 1 of xString after tNumber
		delete char 1 of xString
		repeat while char 1 of xString is among the chars of "0123456789"
			put char 1 of xString after tNumber
			delete char 1 of xString
		end repeat
	end if
	if char 1 of xString is "e" then
		put char 1 of xString after tNumber
		delete char 1 of xString
		if char 1 of xString is among the chars of "+-" then
			put char 1 of xString after tNumber
			delete char 1 of xString
		end if
		repeat while char 1 of xString is among the chars of "0123456789"
			put char 1 of xString after tNumber
			delete char 1 of xString
		end repeat
	end if
	if tNumber is not a number then
		return false
	end if
	seqPushOntoBack xPath["scalars"][the number of elements in xPath["scalars"]], tNumber
	return true
end _svgPathParseNumber

private function _svgParseRectangleValue @xValue
	replace comma with space in xValue
	if the number of words in xValue is not 4 then
		return false
	end if
	local tRectangle
	repeat for each word tCoordinate in xValue
		if not _svgParseLengthValue(tCoordinate) then
			return false
		end if
		seqPushOntoBack tRectangle, tCoordinate
	end repeat
	put tRectangle into xValue
	return true
end _svgParseRectangleValue

private function _svgParseNormalizedRectangleValue @xValue
	if not _svgParseRectangleValue(xValue) then
		return false
	end if
	/* If the width or height of the rectangle is negative, then we treat the
	 * rectangle as 0 size. */
	if xValue[3] begins with "-" or xValue[4] begins with "-" then
		put 0 into xValue[3]
		put 0 into xValue[4]
	end if
	return true
end _svgParseNormalizedRectangleValue

/* The <transform> type represents an affine transform:
 *
 *   transform: <transform-list> | <transform-ref> | none
 *
 *   transform-list
 *     : matrix(<a> <b> <c> <d> <e> <f>)
 *     | translate(<tx> [<ty>])
 *     | scale(<sx> [<sx>])
 *     | rotate(<angle> [<cx> <cy>])
 *     | skewX(<angle>)
 *     | skewY(<angle>)
 *
 *   transform-ref
 *     : ref(svg [<x> <y>])
 *
 * The individual transforms are separated by whitespace or comma.
 */
private function _svgParseTransformValue @xValue
	replace comma with space in xValue
	replace "(" with " ( " in xValue
	replace ")" with " ) " in xValue

	local tTransforms
	repeat while xValue is not empty
		local tType
		put word 1 of xValue into tType
		if tType is not among the items of "matrix,translate,scale,rotate,skewX,skewY" then
			return false
		end if
		if word 2 of xValue is not "(" then
			return false
		end if

		local tValues
		put empty into tValues
		repeat with tIndex = 3 to the number of words in xValue
			if word tIndex of xValue is ")" then
				exit repeat
			end if
			if word tIndex of xValue is not a number then
				exit repeat
			end if
			seqPushOntoBack tValues, word tIndex of xValue
		end repeat
		delete word 1 to tIndex of xValue

		local tArity
		put the number of elements in tValues into tArity

		switch tType
		case "matrix"
			if tArity is not 6 then
				return false
			end if
			break
		case "translate"
			if tArity is not 2 then
				if tArity is not 1 then
					return false
				end if
				seqPushOntoBack tValues, 0
			end if
			break
		case "scale"
			if tArity is not 2 then
				if tArity is not 1 then
					return false
				end if
				seqPushOntoBack tValues, tValues[1]
			end if
			break
		case "rotate"
			if tArity is not 3 then
				if tArity is not 1 then
					return false
				end if
				seqPushOntoBack tValues, 0
				seqPushOntoBack tValues, 0
			end if
			break
		case "skewX"
		case "skewY"
			if tArity is not 1 then
				return false
			end if
		end switch

		local tTransform
		put empty into tTransform
		put tType into tTransform[1]
		seqAppend tTransform, tValues
		seqPushOntoBack tTransforms, tTransform
	end repeat

	put tTransforms into xValue

	return true
end _svgParseTransformValue

/* The <paint> type represents a paint:
 *
 *   paint
 *     : none
 *     | currentColor
 *     | <color>
 *     | <reference> [ none | currentColor | <color> ]
 *     | <system paint>
 *
 */
private function _svgParsePaintValue @xValue
	if xValue is "none" then
		put "none" into xValue["type"]
		return true
	end if

	if xValue is "currentColor" then
		put "current" into xValue["type"]
		return true
	end if

	if _svgParseColorValue(xValue) then
		return true
	end if

	local tId, tBackup
	if matchText(xValue, "^url\s*\(\s*#([a-zA-Z][a-zA-Z0-9]*)*\)(.*)$", tId, tBackup) then
		put word 1 to -1 of tBackup into tBackup
		if tBackup is not empty and \
			tBackup is not "none" and \
			tBackup is not "currentColor" and \
			not _svgParseColorValue(tBackup) then
			return false
		end if
		if tBackup is empty then
			put "none" into tBackup
		end if
		put "reference" into xValue["type"]
		put tId into xValue["id"]
		put tBackup into xValue["fallback"]
		return true
	end if

	if xValue is among the items of \
		("ActiveBorder,ActiveCaption,AppWorkspace,Background,ButtonFace,ButtonHighlight," & \
			"ButtonShadow,ButtonText,CaptionText,GrayText,Highlight,HighlightText,InactiveBorder," & \
			"InactiveCaption,InactiveCaptionTextx,InfoBackground,InfoText,Menu,MenuTextx,Scrollbar," & \
			"ThreeDDarkShadow,ThreeDFace,ThreeDHighlight,ThreeDLightShadow,ThreeDShadow,Window," & \
			"WindowFrame,WindowText") then
		put xValue into xValue["id"]
		put "system" into xValue["type"]
		return true
	end if

	return false
end _svgParsePaintValue

private function _svgParseReferenceValue @xValue
	local tId
	if matchText(xValue, "^url\s*\(\s*#([a-zA-Z][a-zA-Z0-9]*)\s*\)$", tId) then
		put tId into xValue
		return true
	end if

	if xValue begins with "#" then
		put char 2 to -1 of xValue into xValue
		return true
	end if

	return false
end _svgParseReferenceValue

private function _svgParseTextValue @xValue
	return true
end _svgParseTextValue

private function _svgParseMiterLimitValue @xValue
	if _svgParseNonNegativeNumberValue(xValue) and \
		xValue >= 1 then
		return true
	end if
	return false
end _svgParseMiterLimitValue

private function _svgParseIdentifierValue @xValue
	if matchText(xValue, "[_a-zA-Z][_0-9a-zA-Z]*") then
		return true
	end if
	return false
end _svgParseIdentifierValue

private function _svgParseBooleanValue @xValue
	if xValue is among the items of "true,false" then
		put xValue is "true" into xValue
		return true
	end if
	return false
end _svgParseBooleanValue

private function _svgParsePointsValue @xValue
	replace comma with space in xValue

	if (the number of words in xValue) mod 2 is 1 then
		return false
	end if

	local tPoints
	repeat for each word tCoordinate in xValue
		if not _svgParseLengthValue(tCoordinate) then
			return false
		end if
		seqPushOntoBack tPoints, tCoordinate
	end repeat
	put tPoints into xValue

	return true
end _svgParsePointsValue

/* The <opacity> type is a number between 0.0 and 1.0, any values outside of
 * this range are clamped. */
private function _svgParseOpacityValue @xValue
	put word 1 to -1 of xValue into xValue

	local tIsPercentage
	if the last char of the last word of xValue is "%" then
		put true into tIsPercentage
		delete the last char of the last word of xValue
	end if

	if _svgParseNumberValue(xValue) then
		if tIsPercentage then
			put xValue / 100 into xValue
		end if
		put max(min(xValue, 1.0), 0.0) into xValue
		return true
	end if

	return false
end _svgParseOpacityValue

private function _svgParseDashArrayValue @xValue
	if xValue is "none" then
		put empty into xValue
		return true
	end if

	local tLengths
	put empty into tLengths
	replace comma with space in xValue
	repeat for each word tLength in xValue
		if not _svgParseNonNegativeLengthValue(tLength) then
			return false
		end if
		seqPushOntoBack tLengths, tLength
	end repeat

	put tLengths into xValue
	return true
end _svgParseDashArrayValue

/*******************************************************************************
 *
 *  SVG CONTEXT OPERATIONS
 *
 ******************************************************************************/

/* _svgContextEnter pushes the (XML) path to the element onto the path stack.
 * This is used to report the location of any errors / warnings. */
private command _svgContextEnter @xContext, pNode
	seqPushOntoBack xContext["path"], format("%s[%u]", pNode["type"], pNode["index"])
end _svgContextEnter

/* _svgContextLeave pops the most recent element from the path stack. */
private command _svgContextLeave @xContext, pNode
	seqPopFromBack xContext["path"]
end _svgContextLeave

/* _svgContextDefine maps pId to the specified element in an internal mapping.
 * If the id is already defined, the false is returned; otherwise true.
 * The element is a copy, not a reference so redefinition is required if the
 * source element changed. */
private function _svgContextDefine @xContext, pId, pElement
	if pId is among the keys of xContext["ids"] then
		return false
	end if

	put pElement into xContext["ids"][pId]
	return true
end _svgContextDefine

/* _svgContextLookup lookups an element with id pId. If one is found it is
 * returned in rElement and true is returned, otherwise false is returned. */
private function _svgContextLookup @xContext, pId, @rElement
	if pId is not among the keys of xContext["ids"] then
		return false
	end if

	put xContext["ids"][pId] into rElement
	return true
end _svgContextLookup

private command _svgContextSave @xContext
	seqPushOntoBack xContext["state"], seqLast(xContext["state"])
end _svgContextSave

private command _svgContextRestore @xContext
	seqPopFromBack xContext["state"]
end _svgContextRestore

private command _svgContextSetState @xContext, pState, pValue
	put pValue into xContext["state"][the number of elements in xContext["state"]][pState]
end _svgContextSetState

private function _svgContextGetState pContext, pState
	return pContext["state"][the number of elements in pContext["state"]][pState]
end _svgContextGetState

private function _svgContextGetCurrentState pContext
	return seqLast(pContext["state"])
end _svgContextGetCurrentState

private function _svgContextGetLastState pContext
	return pContext["last-state"]
end _svgContextGetLastState

private command _svgContextSetLastState @xContext, pState
	put pState into xContext["last-state"]
end _svgContextSetLastState

private command _svgContextEmit @xContext, pOperation
	seqPushOntoBack xContext["operations"], pOperation
end _svgContextEmit

/* _svgContextUnknownElementError formats an appropriate error message for when
 * encountering an unknown element. */
private command _svgContextUnknownElementError @xContext, pElementType
	_svgContextError xContext, format("element type '%s' unknown - ignoring", pElementType)
end _svgContextUnknownElementError

/* _svgContextUnknownElementError formats an appropriate error message for when
 * encountering an unknown element. */
private command _svgContextUnknownFeatureError @xContext, pFeatureName, pElementType
	_svgContextError xContext, format("feature '%s' on element type '%s' unknown - ignoring", pFeatureName, pElementType)
end _svgContextUnknownFeatureError

private command _svgContextUnknownStylePropertyError @xContext, pFeatureName, pElementType
	_svgContextError xContext, format("style property '%s' on element type '%s' unknown - ignoring", pFeatureName, pElementType)
end _svgContextUnknownStylePropertyError

private command _svgContextInvalidValueForFeatureError @xContext, pFeatureName, pElementType
	_svgContextError xContext, format("invalid value for feature '%s' on element '%s' - ignoring", pFeatureName, pElementType)
end _svgContextInvalidValueForFeatureError

private command _svgContextInvalidDefaultValueError @xContext, pFeatureName, pElementType
	_svgContextError xContext, format("invalid default value for feature '%s' on element '%s' - ignoring", pFeatureName, pElementType)
end _svgContextInvalidDefaultValueError

private command _svgContextElementAlreadyDefinedError @xContext, pId
	_svgContextError xContext, format("element with id '%s' already defined - ignoring redefinition", pId)
end _svgContextElementAlreadyDefinedError

private command _svgContextElementNotDefinedError @xContext, pId
	_svgContextError xContext, format("element with id '%s' not defined - ignoring reference", pId)
end _svgContextElementNotDefinedError

/* _svgContextError reports an error message, augmenting it with the current
 * path in the context. */
private command _svgContextError @xContext, pMessage
	get xContext["path"]
	combine it with slash
	_log format("[SVG ERROR] %s: %s", it, pMessage)
end _svgContextError

/*******************************************************************************
 *
 *  SVG SPECIFICATION OPERATIONS
 *
 ******************************************************************************/

/* sSvgSpec is an array holding information about how to parse SVG documents.
 *
 * The 'elements' key maps element name to an array describing the element.
 * All elements have a 'name' key.
 *
 * The element with the empty name has a key 'property' which contains a map
 * from property name to a feature array.
 *
 * All other elements have an 'attribute' and a 'property' key. The 'attribute'
 * key contains a map from attribute name to a feature array. This describes all
 * the attributes which are applicable to the element.  The 'property' key
 * contains an array, the keys of which describe the properties which apply to
 * the element.
 *
 * A feature array has a 'name' key holding the feature's name, a 'type' key
 * holding the feature's type, a 'nullable' key which determines whether the
 * attribute can be missing, and a 'default' key (if nullable is false) holding
 * the default value of the attribute.
 *
 * Note: We use the element with no name as a store for the actual property
 * definitions. This works because (unlike attributes) properties are global,
 * whereas attributes are local to the element.
 */
local sSvgSpec

/* svgSpecIsElement returns true if pElement is the name of a known element. */
private function svgSpecIsElement pElement
	return pElement is among the keys of sSvgSpec["elements"]
end svgSpecIsElement

/* svgSpecIsProperty returns true if pProperty is the name of a known property. */
private function svgSpecIsProperty pProperty
	return pProperty is among the keys of sSvgSpec["elements"][""]["property"]
end svgSpecIsProperty

/* svgSpecGetColor returns the color values for the given named color and
 * returns true if found, false otherwise. */
private function svgSpecGetColor pColor, @rRed, @rGreen, @rBlue
	if pColor is not among the keys of sSvgSpec["colors"] then
		return false
	end if
	put sSvgSpec["colors"][pColor]["red"] into rRed
	put sSvgSpec["colors"][pColor]["green"] into rGreen
	put sSvgSpec["colors"][pColor]["blue"] into rBlue
	return true
end svgSpecGetColor

/* svgSpecIsPropertyInheritable returns true if the value of a property cascades
 * to children when the property is unset, or whether it has the default
 * set when unset. */
private function svgSpecIsPropertyInheritable pProperty
	/* All properties are inheritable at the moment */
	return sSvgSpec["elements"][""]["property"][pProperty]["inheritable"]
end svgSpecIsPropertyInheritable

/* svgSpecGetApplicableAttributesOfElement returns an array whose keys are the
 * attributes which apply to the given element. */
private function svgSpecGetApplicableAttributesOfElement pElement
	get the keys of sSvgSpec["elements"][pElement]["attribute"]
	split it by return as set
	return it
end svgSpecGetApplicableAttributesOfElement

/* svgSpecIsAttributeApplicableToElement returns true if pAttribute applies to
 * pElement. */
private function svgSpecIsAttributeApplicableToElement pAttribute, pElement
	return pAttribute is among the keys of sSvgSpec["elements"][pElement]["attribute"]
end svgSpecIsAttributeApplicableToElement

/* svgSpectIsAttributeRequiredForElement returns true if the pAttribute is a
 * nullable attribute on pElement. */
private function svgSpecIsAttributeRequiredForElement pElement, pAttribute
	return not sSvgSpec["elements"][pElement]["attribute"][pAttribute]["nullable"]
end svgSpecIsAttributeRequiredForElement

/* svgSpecGetAttributeDefaultForElement returns the default value for attribute
 * pAttribute on pElement. */
private function svgSpecGetAttributeDefaultForElement pElement, pAttribute
	return sSvgSpec["elements"][pElement]["attribute"][pAttribute]["default"]
end svgSpecGetAttributeDefaultForElement

/* svgSpecGetPropertyDefaults returns an array mapping property name to
 * property default value. This is used as the start of the property cascade. */
private function svgSpecGetPropertyDefaults
	local tDefaults
	repeat for each element tProperty in sSvgSpec["elements"][""]["property"]
		put tProperty["default"] into tDefaults[tProperty["name"]]
	end repeat
	return tDefaults
end svgSpecGetPropertyDefaults

private function svgSpecGetDefaultValueForProperty pProperty
	return sSvgSpec["elements"][""]["property"][pProperty]["default"]
end svgSpecGetDefaultValueForProperty

/* svgSpecIsPropertyApplicableToElement returns true if pProperty applies to
 * pElement. */
private function svgSpecIsPropertyApplicableToElement pProperty, pElement
	return pProperty is among the keys of sSvgSpec["elements"][pElement]["property"]
end svgSpecIsPropertyApplicableToElement

/* svgSpecIsPropertySettableOnElement returns true if pProperty can be set on
 * pElement. There are some properties (such as 'fill') which are actually
 * attributes in SMIL, so SMIL nodes don't allow the fill property to be set. */
private function svgSpecIsPropertySettableOnElement pProperty, pElement
	return svgSpecIsProperty(pProperty)
end svgSpecIsPropertySettableOnElement

/* svgSpecIsFeatureApplicableToElement returns true if pFeature can be set on
 * elements of type pElement. */
private function svgSpecIsFeatureSettableOnElement pFeature, pElement
	return svgSpecIsPropertySettableOnElement(pFeature, pElement) or \
			svgSpecIsAttributeApplicableToElement(pFeature, pElement)
end svgSpecIsFeatureSettableOnElement

/* svgSpecGetFeatureTypeForElement returns the type of the given attribute
 * pFeature on pElement; or if pFeature is a property, the type of the property. */
private function svgSpecGetFeatureTypeForElement pFeature, pElement
	get sSvgSpec["elements"][pElement]["attribute"][pFeature]["type"]
	if it is empty then
		get sSvgSpec["elements"][""]["property"][pFeature]["type"]
	end if
	return it
end svgSpecGetFeatureTypeForElement

/* svgSpecLoad loads and parses a description of attributes, and the node types
 * they apply to. */
private command svgSpecLoad
	/* If the sSvgSpec variable is already an array, then the specification has
	 * already been loaded. */
	if sSvgSpec is an array then
		exit svgSpecLoad
	end if

	/* Load the svg specification text file. */
	local tSpecText
	put url ("file:" & _GetResourcesPath() & slash & "drawing-svg-specification.txt") into tSpecText
	if the result is not empty then
		_svgSpecError 0, "failed to read specification"
	end if

	/* Keep track of the current row being processed, and also the current part
	 * which is being parsed. */
	local tCurrentRow, tCurrentElement
	put 0 into tCurrentRow
	put empty into tCurrentElement
	repeat for each line tLine in tSpecText
		/* Keep a running count of the row */
		add 1 to tCurrentRow

		/* Ignore any comment lines (those beginning with '#'). */
		if word 1 of tLine begins with "#" then
			next repeat
		end if

		/* Ignore any empty lines. */
		if word 1 to -1 of tLine is empty then
			next repeat
		end if

		/* The type of line we are parsing is determined by the first word. */
		local tLineType
		put word 1 of tLine into tLineType

		/* An 'element' line finishes the current element (if any) and sets
		 * things up to parse a new one. */
		if tLineType is "element" then
			/* If there is an element currently being processed, then record it
			 * in the sSvgSpec array. */
			if tCurrentElement is an array then
				put tCurrentElement into sSvgSpec["elements"][tCurrentElement["name"]]
				put empty into tCurrentElement
			end if

			/* Element lines have at most two words. */
			if the number of words in tLine > 2 then
				_svgSpecError tCurrentRow, format("invalid element clause")
				next repeat
			end if

			/* The second word of the element line is the element name, and this
			 * must not have been used before. */
			if word 2 of tLine is among the keys of sSvgSpec["elements"] then
				_svgSpecError tCurrentRow, format("element '%s' already defined", word 2 of tLine)
				next repeat
			end if

			/* Record the name of the new element. */
			put word 2 of tLine into tCurrentElement["name"]

			next repeat
		end if

		/* The 'property' and 'attribute' lines have the same format and define
		 * a feature. */
		if tLineType is "property" or tLineType is "attribute" then
			/* If there is no current element, then there is nothing to attach
			 * the feature to. */
			if tCurrentElement is not an array then
				_svgSpecError tCurrentRow, "no current element"
				next repeat
			end if

			/* Properties must only be defined against the element with no
			 * name. */
			if tLineType is "property" and \
				tCurrentElement["name"] is not empty then
				_svgSpecError tCurrentRow, "properties must be defined in the unnamed element"
				next repeat
			end if

			/* The syntax of a feature is:
			 *   (property | attribute ) <name> <type> (nullable | default <value>) [inheritable]
			 * So a feature line must have at least 4 words */
			if the number of words in tLine < 4 then
				_svgSpecError tCurrentRow, "invalid feature clause (too few words)"
				next repeat
			end if

			/* If this is a property, it can have 'inheritable' at the end */
			local tIsInheritable
			put false into tIsInheritable
			if tLineType is "property" then
				if the last word of tLine is "inheritable" then
					put true into tIsInheritable
					delete the last word of tLine
				end if
			end if

			/* Extract the name of the feature from the second word, and the
			 * type of the feature from the third. */
			local tFeature
			put word 2 of tLine into tFeature["name"]
			put word 3 of tLine into tFeature["type"]

			/* The type of a feature is a '|' delimited list of options. */
			split tFeature["type"] by "|"

			/* Extract the nullable / default clause from the line. */
			if word 4 of tLine is "default" then
				/* The feature has a default value, so we expect exactly 5 words
				 * in the line. */
				if the number of words in tLine is not 5 then
					_svgSpecError tCurrentRow, "invalid feature clause (wrong number of words)"
					next repeat
				end if

				/* The fifth word is the feature default value. */
				put word 5 of tLine into tFeature["default"]

				/* We allow the default value to be a quoted string, mainly as
				 * some values have the empty string as default, so remove the
				 * quotes, if present. */
				if tFeature["default"] begins with quote then
					if char -1 of tFeature["default"] is not quote then
						_svgSpecError tCurrentRow, "invalid feature clause (missing end quote on default value)"
						next repeat
					end if

					put char 2 to -2 of tFeature["default"] into tFeature["default"]
				end if

				/* If a feature has a default value, then it is not nullable. */
				put false into tFeature["nullable"]
			else if word 4 of tLine is "nullable" then
				/* The feature has been marked as nullable, so mark it as such. */
				put true into tFeature["nullable"]
			end if

			if tLineType is "property" then
				put tIsInheritable into tFeature["inheritable"]
			end if

			/* The name of a feature must be unique in the current element. */
			if tFeature["name"] is among the keys of tCurrentElement[tLineType] then
				_svgSpecError tCurrentRow, format("feature '%s' already defined for current element", tFeature["name"])
				next repeat
			end if

			/* Add the feature to the current element's feature map. */
			put tFeature into tCurrentElement[tLineType][tFeature["name"]]

			next repeat
		end if

		/* The 'apply' line marks an element as consuming the specified
		 * property. */
		if tLineType is "apply" then
			/* There must be a current element for apply to make sense. */
			if tCurrentElement is not an array then
				_svgSpecError tCurrentRow, "no current element"
				next repeat
			end if

			/* Apply clauses are only allowed on named elements. */
			if tCurrentElement["name"] is empty then
				_svgSpecError tCurrentRow, "'apply' can only be used on named elements"
				next repeat
			end if

			/* For named elements, the property array is a set of applicable
			 * properties. */
			put true into tCurrentElement["property"][word 2 of tLine]

			next repeat
		end if

		/* The 'color' line defines a named color */
		if tLineType is "color" then
			/* Color lines are: 'color' <name> <red> <green> <blue> */
			local tColorName, tColorRed, tColorGreen, tColorBlue
			put word 2 of tLine into tColorName
			put word 3 of tLine into tColorRed
			put word 4 of tLine into tColorGreen
			put word 5 of tLine into tColorBlue

			/* The color must not be previously defined */
			if tColorName is among the keys of sSvgSpec["colors"] then
				_svgSpecError tCurrentRow, format("color '%s' previously defined", tColorName)
				next repeat
			end if

			/* The three color values must all be numbers */
			if tColorRed is not a number or \
				tColorGreen is not a number or \
				tColorBlue is not a number then
				_svgSpecError tCurrentRow, "invalid color values specified"
				next repeat
			end if

			/* Clamp the color values (out of range values should probably be
			 * an error) and put them into the colors key of the spec array. */
			put min(max(tColorRed, 0), 255) into sSvgSpec["colors"][tColorName]["red"]
			put min(max(tColorGreen, 0), 255) into sSvgSpec["colors"][tColorName]["green"]
			put min(max(tColorBlue, 0), 255) into sSvgSpec["colors"][tColorName]["blue"]

			next repeat
		end if

		/* At this point the line type must be unknown. */
		_svgSpecError tCurrentRow, format("unknown line type '%s'", tLineType)
	end repeat

	/* Make sure the last element is recorded in the spec array. */
	if tCurrentElement is an array then
		put tCurrentElement into sSvgSpec["elements"][tCurrentElement["name"]]
	end if
end svgSpecLoad

private command _svgSpecLoadPart pPart
	if pPart["type"] is not "element" then
		if "value" is not among the keys of pPart or \
			"animatable" is not among the keys of pPart or \
			"inheritable" is not among the keys of pPart then
			_svgSpecError pPart["row"], format("incomplete %s definition", pPart["type"])
		end if
	end if

	if pPart["name"] is among the keys of sSvgSpec[pPart["type"]] then
		_svgSpecError pPart["row"], format("%s with name '%s' already definted", pPart["type"], pPart["name"])
	end if

	if "implemented" is not among the keys of pPart then
		put true into pPart["implemented"]
	end if

	put pPart into sSvgSpec[pPart["type"]][pPart["name"]]
end _svgSpecLoadPart

private command _svgSpecError pRow, pMessage
	_log format("[SPEC ERROR] row %d: %s", pRow, pMessage)
	throw "svgerr,error loading svg specification"
end _svgSpecError

/*******************************************************************************
 *
 *  XML OPERATIONS
 *
 ******************************************************************************/

/* xmlImportFromFile converts an XML document into a LiveCode array, with array
 * structure suitable for parsing SVG XML files (and other, similar W3C-defined
 * XML document formats).
 *
 * Each text XML element is mapped to a string.
 *
 * Each non-text XML element is mapped to an array with 3 keys:
 *   - type: the xml element tag (e.g. svg)
 *   - index: the index of the element type in the parent
 *   - features: an array mapping feature name to value
 *   - content: either a string if there is a single text node, or an XML
 *     element array
 *
 * Note: We use the term 'features' here, rather than 'attributes' as SVG uses
 * XML attributes to specify both SVG attributes and SVG properties.
 *
 * For example:
 *
 *   <?xml version="1.0"?>
 *   <svg xmlns="http://www.w3.org/2000/svg" version="1.2" baseProfile="tiny">
 *   <desc>Demonstrates use of a default namespace prefix for elements.</desc>
 *   <rect width="7" height="3"/>
 *   </svg>
 *
 * Will be imported as the follow array:
 *
 *   svg:
 *     attributes:
 *       xmlns: http://www.w3.org/2000/svg
 *       version: 1.2
 *       baseProfile: tiny
 *     index: 1
 *     content:
 *       1:
 *         desc:
 *           index: 1
 *           content: Demonstrates use of a default namespace prefix for elements.
 *       2:
 *         rect:
 *           index: 1
 *           features:
 *             width: 7
 *             height: 3
 *
 * Any XML errors (generated by libxml) will be thrown; otherwise an array will
 * be returned in the above structure.
 */
private function xmlImportFromFile pXMLFile
   local tArray

   local tTreeId
   put revCreateXMLTreeFromFileWithNamespaces(pXMLFile, true, true, false) into tTreeId

   if tTreeId begins with "xmlerr," then
      throw tTreeId
   end if

   try
      local tRootNode
      put revXMLRootNode(tTreeId) into tRootNode
      if tRootNode begins with "xmlerr," then
         throw tRootNode
      end if

      put _xmlImportNode(tTreeId, revXMLRootNode(tTreeId)) into tArray

   finally
      revDeleteXMLTree tTreeId
   end try

   return tArray
end xmlImportFromFile

private function xmlImportFromText pXMLText
   local tArray

   local tTreeId
   put revCreateXMLTreeWithNamespaces(pXMLText, true, true, false) into tTreeId

   if tTreeId begins with "xmlerr," then
      throw tTreeId
   end if

   try
      local tRootNode
      put revXMLRootNode(tTreeId) into tRootNode
      if tRootNode begins with "xmlerr," then
         throw tRootNode
      end if

      put _xmlImportNode(tTreeId, revXMLRootNode(tTreeId)) into tArray

   finally
      revDeleteXMLTree tTreeId
   end try

   return tArray
end xmlImportFromText

/* _xmlImportNodeType extracts the XML node tag and index from the path provided
 * by libXML. */
private command _xmlImportNodeTypeAndIndex pNode, @rType, @rIndex
	set the itemDelimiter to "/"
	get the last item of pNode
	set the itemDelimiter to "["
	put item 1 of it into rType
	put char 1 to -2 of item 2 of it into rIndex
	if rIndex is empty then
		put 1 into rIndex
	end if
end _xmlImportNodeTypeAndIndex

/* _xmlImportNode is the name recursive function which imports an XML node in
 * the LiveCode array structure. */
private function _xmlImportNode pTreeId, pNode
   /* Extract the node type and index from the pNode path */
   local tArray
   _xmlImportNodeTypeAndIndex pNode, tArray["type"], tArray["index"]

   -- First extract the attributes, these go into an 'attribute' field
   -- as an array mapping attribute name to value
   local tAttributes
   put revXMLAttributes(pTreeId, pNode, "=", return) into tAttributes
   if tAttributes begins with "xmlerr," then
      throw tAttributes
   end if
   split tAttributes by return and "="
   put tAttributes into tArray["features"]

   -- Get the list of child nodes
   local tChildNodes
   put revXMLChildNames(pTreeId, pNode, return, empty, true, true) into tChildNodes
   if tChildNodes begins with "xmlerr," then
      throw tChildNodes
   end if

   /* If the list of child nodes of the element is not empty, then we
    * recursively import each child node into a sequence; otherwise the node
    * only contains text (or has no content) which we import as a string. */
   local tContent
   if tChildNodes is not empty then
      local tChildIndex
      put 1 into tChildIndex
      repeat for each line tChildLeaf in tChildNodes
         put _xmlImportNode(pTreeId, pNode & slash & tChildLeaf) into tContent[tChildIndex]
         add 1 to tChildIndex
      end repeat
   else
      put revXMLNodeContents(pTreeId, pNode) into tContent
      if tContent begins with "xmlerr," then
         throw tContent
      end if
   end if

   /* The content of the node, whether it be just a string or a sequence of
    * other nodes is placed in the 'content' key of the resulting node array. */
   put tContent into tArray["content"]

   return tArray
end _xmlImportNode

/*******************************************************************************
 *
 *  SEQUENCE OPERATIONS
 *
 ******************************************************************************/

private command seqPushOntoBack @xSeq, pValue
	put pValue into xSeq[the number of elements in xSeq + 1]
end seqPushOntoBack

private command seqPopFromBack @xSeq
	delete variable xSeq[the number of elements in xSeq]
end seqPopFromBack

private command seqAppend @xSeq, pOther
	repeat for each element tElement in pOther
		seqPushOntoBack xSeq, tElement
	end repeat
end seqAppend

private function seqLast pSeq
	return pSeq[the number of elements in pSeq]
end seqLast

/*******************************************************************************
 *
 *  INTERNAL OPERATIONS
 *
 ******************************************************************************/

private function _GetResourcesPath
   local tResourcesFolder
   -- in the mapping, tResourcesFolder begins with ./
   try
      put char 3 to -1 of revLibraryMapping[the short name of me & \
         slash & "resources"] into tResourcesFolder
   end try
   if tResourcesFolder is empty then
      set the itemDelimiter to slash
      return item 1 to -2 of the filename of me & slash & "resources"
   else
      put specialFolderPath("resources") & slash before tResourcesFolder
      return tResourcesFolder
   end if
end _GetResourcesPath

private function _ArrayToString pArray, pPrefix
   local tString
   if "1" is among the keys of pArray then
      local tIndex
      put 1 into tIndex
      repeat for each element tElement in pArray
         put pPrefix & "[" & tIndex & "]" & space after tString
         if tElement is an array then
            put return & _ArrayToString(tElement, pPrefix & "  ") after tString
         else
            put tElement after tString
         end if
         put return after tString
         add 1 to tIndex
      end repeat
   else
      repeat for each key tKey in pArray
         put pPrefix & tKey & ":" & space after tString
         if pArray[tKey] is an array then
            put return & _ArrayToString(pArray[tKey], pPrefix & "  ") after tString
         else
            put pArray[tKey] after tString
         end if
         put return after tString
      end repeat
   end if
   delete the last char of tString
   return tString
end _ArrayToString

private command _InternalError pMessage
	throw pMessage
end _InternalError

private command _LogArray pArray
	if the environment is "command line" then
		_Log _ArrayToString(pArray)
	end if
end _LogArray

private command _Log pMessage
	if the environment is "command line" then
		write pMessage & return to stderr
	else
		--put pMessage & return after msg
	end if
end _Log

/******************************************************************************/
