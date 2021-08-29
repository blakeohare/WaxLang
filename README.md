# Wax

Wax is an experiment in progress to create a programming language and framework that allows you to create applications that can be structured like microservices, called Wax Components. These components can communicate to each other via JSON-like requests and responses. 

## Exporting App Bundles

The final output of Wax is a bundle of components. One of the component is designated as the Main Component and makes calls to the other components. 

A Wax Component can be written using WaxLang, which has no platform-dependent features (such as graphics or input) and is good for writing pure business logic. Alternatively, you can also write a Wax component for specific platforms, using languages such as JavaScript, or PHP, or C with interesting dependencies. The source codes for these various platforms co-exist with each other in your code base. When you export your app bundle, you can choose for which platform you want to export.

Additionally, components do not need to be included with the app bundle you export. You can instead configure the bundle to point to an extension server where the components can be downloaded as on-demand extensions. This allows you to run and test large programs with many different features like a single large executable, but distribute only a small version of it and allow customers to only download the parts that they need.

Another way to split the final app bundle is by exporting Wax Components into server components and the app bundle itself points to a specific server URL. This allows you to either run your program locally or offer it as a SaaS application. This is also a good way to include behavior that is not allowed by the current platform. 

## Use case

Suppose you wanted to create a program that creates Android APK files. This requires the Android SDK to be downloaded and installed. You could create an app component that generates the source code of the Android project you wish to create. You could then create a platform-specific native component that runs the source code through Gradle to create an APK via the command line. When exporting this app-creating-app, you have a few choices:

* You can create a purely native bundle where both of these components are included in one executable.
* You can create a http-dependent native version where the app bundle makes an HTTP request instead of an in-process request to a server (the server component is also exported and you have to run it yourself)
* You can create a fully web-based version where the app creator app now runs in the browser, and makes the requisite backend calls to generate the APK.

# Status

This project is currently being built and tinkered with. It is not usable yet. There is a sample project called PrimeExample that demonstrates the structure of Wax projects.
