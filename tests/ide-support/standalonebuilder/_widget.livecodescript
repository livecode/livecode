script "TestWidgetStandalone"
on _testWidget
  get the cWidgetKind of me
  write it to stderr
  create widget as it
end _testWidget

on startup
  set the lockErrorDialogs to true
  try
	 _testWidget
  catch tError
	 write "widget creation failed:" && tError to stderr
	 quit 1
  end try
  
  DoQuit
end startup

on DoQuit
  if "errorDialog" is in the pendingmessages then
    send "DoQuit" to me in 0 millisecs
  else
    quit 0
  end if
end DoQuit

on errorDialog pError
	write "widget creation failed:" && pError to stderr
	quit 1
end errorDialog