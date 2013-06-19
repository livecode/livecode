# Additional statistical functions
The following statistical functions were added to the engine:
* 'averageDeviation' (or shorter: 'avgDev')
* 'populationStandardDeviation' (or shorter: 'popStdDev')
* 'populationVariance' (or shorter: 'popVariance')
* 'sampleVariance' (or shorter: 'sampVariance' or 'variance')
* 'geometricMean'
* 'harmonicMean'

The following synonyms for existing statistical functions were added to clarify their meaning:
* 'arithmeticMean' and 'mean' as synonyms for 'average'
* 'sampleStandardDeviation' and 'sampStdDev' for 'stdDev'

For a variable `theNumbers` containing a series of numbers:
`2,4,4,4,5,5,7,9`
these statistical functions return the following values:
* **averageDeviation(theNumbers)** -> `1.5`
* **populationStandardDeviation(theNumbers)** -> `2`
* **sampleStandardDeviation(theNumbers)** -> `2.13809`
* **populationVariance(theNumbers)** -> `4`
* **sampleVariance(theNumbers)** -> `4.571429`
* **arithmeticMean(theNumbers)** -> `5`
* **geometricMean(theNumbers)** -> `4.603216`
* **harmonicMean(theNumbers)** -> `4.201751`
