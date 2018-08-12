script "EnvironmentUnlicensedBuyNowButtonBehavior"
on mouseUp
   switch the short name of this card
      case "Trial Expired"
         doBuyLiveCode
         break
      case "Upgrade Required"
         doUpgradeLiveCode
         break
      case "Membership Required"
         doJoinLiveCode
         break
      case "Update Required"
         doUpdateLiveCode
         break
   end switch
end mouseUp

on mouseDown
end mouseDown
