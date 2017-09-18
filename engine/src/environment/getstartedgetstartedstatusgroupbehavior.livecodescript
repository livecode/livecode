script "EnvironmentGetStartedGetStartedStatusGroupBehavior"
on selectIndicator pStep
   set the backgroundColor of graphic "accountIndicator" of me to white
   set the backgroundColor of graphic "introductionIndicator" of me to white
   set the backgroundColor of graphic "getStartedIndicator" of me to white
   
   set the backgroundColor of graphic (pStep & "Indicator") of me to "173,208,54"
end selectIndicator
