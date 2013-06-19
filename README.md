# What is LiveCode?

LiveCode helps you create apps for multiple platforms.
Quickly. Easily. Free.

LiveCode brings three key things to your development process: the interface, the language and live coding. 
You start with the interface. Get going by opening a new project and dragging items onto it. A button, a field, a scrollbar… whatever your app needs. 
Play about with them a bit. Resize, rearrange, change the color, add some pretty drop shadows. Make it look funky and just the way you want it.

Now the language comes in. It’s English. You add it to your objects to make them do what you want them to do. For example writing:

    on mouseUp
    answer “Hello World”
    end mouseUp

will make a dialog pop up that says Hello World when you click a button. Simple.

Finally, you get to test the code the minute you write it. Switch to run mode, click your button, and if it doesn’t say “Hello World” you know something went wrong. Go back and fix it. No big deal, no time lost, and this live process means that you can quickly find out where the error crept in. 

For more information visit http://livecode.com

# Open source

Thi is LiveCode Community, and it is an Open Source application, this means that you can look at and edit all of the code used to run it, including the engine code. Of course, you do not have to do this, if you just want to write your app in LiveCode there is no need for you to get involved with the engine at all. You write your app using LiveCode with the English-like scripting language, and our drag and drop interface. Fast, easy, productive and powerful.

Any apps you create using LiveCode Community must themselves be open source. You can use it any way you wish, you can even sell what you make, but you must make your source code public under the GPL license. You can do this by including it with your distributed app, or including a link to it that users can click on and view your code. This makes LiveCode Community perfect for educators and students, hobbyists or any use where you are not concerned about sharing your source code. If at any time you want to create a professional app and protect your code, you can purchase a Commercial license for LiveCode which allows you to build a closed source app.

For commercial lincenses visit: http://livecode.com/store/ 

# Features

## Simple Easy to Learn Scripting Language

Way back in the dawn of computer history, programming languages were 1’s and 0’s. Now, while you can still hunt down your mammoth (well, ok, rabbit) using a bow and arrow, skin and joint it, take it home and cook it, its a lot easier just to go to the supermarket. Or even the takeaway. And while you can still use 1’s and 0’s, in 99.999% of cases, its not a great way to program. LiveCode’s language is a natural programming language that starts from the premise “wouldn’t it be nice just to tell my program what I want it to do. In English”. It is designed to be expressive, readable, memorable and as close as possible to the way you speak and think. And like real speech, it’s not limiting. It has all the elements needed to express powerful programming concepts, in a simple straightforward way.

This isn’t just a better way to write your programs, its a better way to read them as well. Programmers probably spend more time reading and trying to understand the code they have written than they do actually writing it.

For example, take a look at this piece of code (this is NOT LiveCode)
     
     theText = theText.split("\n");
     theText = theText.sort(sort_item_3).join("\n");
     function sort_item_3(line1, line2) {
       line1 = line1.split(",");
       line2 = line2.split(",");
       if(line1[2] == line2[2]) return 0;
      else if(line1[2] &amp;gt; line2[2]) return -1;
      else return 1;
      }

Or the exact same line of LiveCode:

    sort lines of theText descending by last item of each

Which are you more likely to understand on first glance 6 months after you wrote it? LiveCode is the only language which is this straightforward.

## Visual Environment

LiveCode’s visual development environment makes creating your user interface quick and easy. Create a new stack and drag the controls you need over from the Tools Palette onto your stack. Place them where you want, resize them and change their properties to change their appearance and behavior.

Once you have your interface use the built in Code Editor to add functionality to your app. LiveCode informs your app when user interactions occur. So a button knows when it is clicked, a field knows when it is typed in and a slider know when its value changes. Use LiveCode’s English-like language to tell your controls how to respond to user actions.

Because LiveCode is live, you can see how your app will look as you create it, making small changes as you go.

## Live editing and testing

Now you have a scripted interface, does it work? In LiveCode your application is always running, even while developing. You can add new objects, or change the code while your application is live and instantly see the effect. No other language does this. It makes it incredibly easy to develop quickly, making one change and trying it out. Editing live applications creates a powerful workflow, so you can build your apps faster and easier than ever.

To create this live process, LiveCode has two modes. The Edit mode allows you to select controls, move them around, change properties and edit code. The Run mode allows you to interact with your app as a user would. LiveCode allows you to switch between these two modes instantly. No waiting around for your app to compile or build.

## Introducing the Best Text and Data Processing in the World

It’s a bold claim. It’s one of LiveCode’s greatest strengths. Unique among multiplatform programming languages, LiveCode understands text in the same way you do: characters, words and lines. This can save you lots of time, money and effort.

Text and data processing is a component in virtually every application or IT solution. In many applications it’s the core functionality, a major component or one of the main outputs. Unstructured data comes from many sources, commonly appearing in e-mails, news, Web pages, memos, notes from call centers and support operations, chat logs, system logs, surveys, user forums and reports.

Unstructured data is not always easy to find, access, analyze or use. LiveCode makes it effortless. 

## We’re all about Data

You need data. Your app needs data. Your boss needs data. Your client needs data. LiveCode does data. For simple needs like an address book or small data sets, you can just use LiveCode itself as a container. For more complex needs, LiveCode works as a great front end for your databases. And if you’re talking Big Data, LiveCode offers unique flexibility for drilling into and custom managing data sets.

LiveCode connects easily to web services or information based in the cloud. It also directly connects to MySQL and SQLite on all platforms including Mobile, and supports PostgreSQL, Valentina and Oracle on the desktop. We support ODBC so you can use this standard protocol to connect to other types of SQL databases such as Microsoft SQL Server.

## Beautiful Apps that Sing and Dance

While LiveCode is not Photoshop, you’ll be surprised at how much you can do from within the program without leaving it. LiveCode’s suite of multimedia features includes everything you need to create stunning content. Create exquisite user interfaces with complete control over skinning. Add graphic effects such as drop shadows and glows, object blends or gradient fills. Apply deep masks to create custom, translucent window shapes. Effortlessly import assets to use in your app. Bring it all together with audio and video, visual effects and animation.

## Web Aware

Your app is not alone in the world. It can talk to the Web. Support for standard Internet protocols is built in to LiveCode. Access http, https, ftp, and http post URLs starting from one line of code. For example, to download an image from the Web and render it into an image object in your application:
 
    put URL "http://www.xyz.com/image.jpg" into image "Viewer"

## Open and Shut

LiveCode plays nice with other programs. Support is built-in for launching applications, launching documents, opening processes, reading and writing to other processes, or executing shell commands. The interprocess communication feature set makes it easy to interact with other applications or processes running on the same machine.

For example, to load a Web browser with a URL:

    launch URL "http://www.runrev.com/"

## Run out of features? Impossible.

You really can’t do what you need with our existing feature set? The magic doesn’t end there. Because we offer LiveCode Community, the totally open source edition of LiveCode, you can get right into the engine and add your own features. Find out more here. [how to contribute link]

You can also extend LiveCode using modules written in lower level languages such as C. These are called “externals”

# Cross-platform apps

LiveCode is ideal for building cross platform apps. We’ve worked hard to make the language as compatible as possible and to minimize the number of changes you need to make on each platform. There are many cases where LiveCode applications will run on different platforms with no changes at all.

The Language Learning Specialists at EuroTalk use LiveCode to create best selling apps for both the desktop and mobile devices.

## Native support for each platform

LiveCode has support for the native features on each platform that we support.

To take full advantage of them, you can design your app to switch user interface elements and layouts depending on the platform it is running on.

Typically applications will run on Windows, Mac and Linux desktops with minimal modification. LiveCode allows you to tweak the user experience on each platform.

Mobile apps currently require the use of a library (such as MobGUI) to create a native experience. Multimedia or custom themed mobile apps do not. Built in native theming is part of our road map, we’ll be adding it shortly.

You can often reuse LiveCode language libraries that you developed for other platforms in LiveCode server.

## Code once and deploy to all platforms

LiveCode’s cross platform feature allows the same code base to be deployed to Android, iOS, Mac, Windows or Linux at the click of a button. This means that you can make your app accessible to everyone at minimal effort or cost.

## Multi-tiered, multi-component solutions

LiveCode fits well as a component within a complex system. Your LiveCode client app can connect to a pre-existing web or cloud service. Or your LiveCode server app can serve content to an app written in another language. A LiveCode application can serve as a front end calling code libraries written in other languages or drive other applications through sockets, the command line or inter-process communication.
