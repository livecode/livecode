script "InstallerInstallerUIUpdateCheckUpdatesAvailableMarketingFieldBehavior"
setProp uFormattedTextArray pNewText
   local tMktIndex
   put 1 into tMktIndex
   put empty into me
   repeat while pNewText[tMktIndex]["content"] is not empty
      switch pNewText[tMktIndex]["type"]
         case "html" 
            set the HTMLText of char (the number of chars in me + 1) of me to pNewText[tMktIndex]["content"]
            break
         case "rtf" 
            set the RTFText of char (the number of chars in me + 1) of me to pNewText[tMktIndex]["content"]
            break
         case "unicode"
            set the unicodeText of char (the number of chars in me + 1) of me to pNewText[tMktIndex]["content"]
            break
         case "utf8" 
            set the unicodeText of char (the number of chars in me + 1) of me to unidecode(pNewText[tMktIndex]["content"], "UTF8")
            break
         case "plain"
         default
            set the text of char (the number of chars in me + 1) of me to pNewText[tMktIndex]["content"]
            break            
      end switch
      put return after of me
      add 1 to tMktIndex
   end repeat
end uFormattedTextArray
