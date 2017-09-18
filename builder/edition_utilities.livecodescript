script "EditionUtilities"
constant kEditionNames="community,communityplus,indy,business,commercial"
function editionNames
   return kEditionNames
end editionNames

function editionTitleCase pEdition
	switch pEdition
	case "community"
		return "Community"
	case "communityplus"
		return "CommunityPlus"
	case "indy"
		return "Indy"
	case "business"
		return "Business"
	case "commercial"
		return "Commercial"
	end switch
end editionTitleCase

function editionDisplayName pEdition
	switch pEdition
	case "community"
		return "Community"
	case "communityplus"
		return "Community Plus"
	case "indy"
		return "Indy"
	case "business"
		return "Business"
	case "commercial"
		return "Commercial"
	end switch
end editionDisplayName


function editionCompare pA, pB
	local tA, tB
	put itemOffset(pA, kEditionNames) into tA
	put itemOffset(pB, kEditionNames) into tB
	return tA - tB
end editionCompare

constant kCommercialBaseEditions="communityplus,indy,business,commercial"
function editionBaseEditionType pEdition
	if pEdition is among the items of kCommercialBaseEditions then
		return "Commercial"
	else
		return "Community"
	end if
end editionBaseEditionType

constant kPrivateRepoEditions="communityplus,indy,business,commercial"
function editionIsInPrivateRepo pEdition
   return pEdition is among the items of kPrivateRepoEditions
end editionIsInPrivateRepo

