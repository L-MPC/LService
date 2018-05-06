# LService
Secure Chat Service Based On Tox Core

How to Build :
->Open SampleService.sln
->Change build to the Release Version instead of Debug
->Build SampleService.sln
->Outputs the LService.exe at the same folder of SampleService.sln
->The LService requires the libtox.dll to be at the same folder with the LService.exe while it runs

About :
The LService is a simplified chat system service using ToxCore library for secure communications 
and can be used by other applications via intercoms or any other internal or external coms way,
that`s why it doesn`t have a user interface.
It can be used as a wrapper of ToxCore library for the very and most basic functions for 
a secure chat system in windows operating systems by any application.
It is running stable in windows systems and is hidden but visible through the system processes.
It can be easy become a native windows service too instead of running as hidden executable service.
This service is under GPL3 license since it makes use of ToxCore library for windows which is under the same License.

For a complete secure Chat system with Graphical interface have a look at the below Tox clients:
https://tox.chat/
https://tox.chat/clients.html
https://github.com/irungentoo/toxcore


