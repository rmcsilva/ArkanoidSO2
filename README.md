# ArkanoidSO2
Multi-player clone of Arkanoid using Visual Studio ![platform](https://img.shields.io/badge/platform-win--32%20win--64-lightgrey)

## Projects
### ClientGUI
This project has the GUI of the client and it is the one you need to play the game.

![login](https://user-images.githubusercontent.com/33632491/69048236-e82aa300-09f4-11ea-87e3-0e57235ae402.png)

First you need to connect to the server with your username. If the server is running on your machine you don't need to put anything on the Server IP box.
After loging in you will get a message to let you know if everything went according to plan or what the problem was.

![initialBackground](https://user-images.githubusercontent.com/33632491/69048608-cda4f980-09f5-11ea-8d0b-48f07027cdb1.png)

At this point you can wait for server to start the game or you can:
* Switch the keys you use to move your paddle

  ![keySettings](https://user-images.githubusercontent.com/33632491/69048803-4906ab00-09f6-11ea-9458-ec160e9cb3a5.png)

 * Request the Top10 so you know what score you need to crush so your username is the first one on that list
 
    ![top10](https://user-images.githubusercontent.com/33632491/69048886-781d1c80-09f6-11ea-9f40-eb114316d53b.png)
 
 * Take a glipmse at this amazing About section
 
    ![about](https://user-images.githubusercontent.com/33632491/69048983-b74b6d80-09f6-11ea-86fa-e3f7e7d8c8ef.png)
 
 
 Now that your setup is done the game should be ready to start
 
 ![arkanoidGame](https://user-images.githubusercontent.com/33632491/69049040-dc3fe080-09f6-11ea-9342-63ca9cfac211.png)
 
 You can move the paddle around with your mouse using a constant circle motion to the place you want to go. Or you can use the keys you choose on the settings page or the default ones (A -> Move Left, D -> Move Right)
 
 All you need to do now is enjoy the game!
 
 ### Server
 The server is the brains behind the game and it handles everything from the client connection and requests to the game logic.
 The connection is done through shared memory or named pipes, depending if the client is local or remote, with the help of mutexs and semaphores to ensure everything is synchronized.
 
 ![server](https://user-images.githubusercontent.com/33632491/69049200-3ccf1d80-09f7-11ea-9380-ba677dc1086f.png)
 
  After the server started running it is possible to:
  * Start or end the game
  * Show the amazing Top10 with all it's glorious users 
  * List all the connected users
  * End the server along with all the clients that are connected
  
  
 ### DLL
 There is also a DLL that is implicitly added to the client and the server.
 Its job is to encapsulate the communication between the client and the server and it stores the data structures that are used by both.
 
 #### Assignment Specifications
 You can find the assignment specifications in portuguese [here](https://github.com/rmcsilva/ArkanoidSO2/blob/master/SO2%20-%201819%20-%20Enunciado%20do%20Trabalho.pdf) 
 
