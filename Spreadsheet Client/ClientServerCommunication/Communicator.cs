/*
  File: Communicator.cs
  Author: Alexander Anderson
  Team: SegFault
  CS 3505 - Spring 2015
  Date created: April 1, 2015
  Last updated: April 20, 2015
   
   
  Resources:
  - StringSocketOfficial.dll from CS3500 Fall 2014
   
*/
  using CustomNetworking;
  using System;
  using System.Collections.Generic;
  using System.Linq;
  using System.Net;
  using System.Net.Sockets;
  using System.Text;
  using System.Threading;
  using System.Threading.Tasks;

  namespace ClientServerCommunications
  {
    /// <summary>
    /// Client-Server communication class.
    /// It handles incoming/outgoing messages between the client and the server.
    /// </summary>
    public class Communicator
    {
        private TcpClient client;
        private StringSocket socket = null;
        private object payload = new object();

        public delegate void UpdateCell(string cell, string contents);
        private UpdateCell updateCell;

        public delegate void ErrorCB(int error, string info);
        private ErrorCB error;

        public delegate void isEditable();
        private isEditable editable;
        private int remainingCellsForInit;

        /// <summary>
        /// Constructs the Communicator class and registers callbacks
        /// No commands are sent.
        /// 
        /// Definitions:
        /// UpdateCell -        Callback that sends back successful cell edits.
        /// ErrorCB -           Callback that sends back error codes and information about the error.
        /// isEditable-         Callback that initiates when all cells have been received from the initial cell population.
        /// 
        /// </summary>
        /// <param name="port"></param>
        /// <param name="serverIP"></param>
        /// <param name="e"></param>
        public Communicator(UpdateCell c, ErrorCB error, isEditable editable)
        {
            //Registering all callbacks
            updateCell = c;
            this.error = error;
            this.editable = editable;
        }

        /// <summary>
        /// Parses incoming server message and sends out appropriate response to client via callbacks.
        /// 
        /// Possible server messages:
        /// connected number_of_cells
        /// cell cell_name cell_contents
        /// error error_ID error_additional_information
        /// </summary>
        /// <param name="s"></param>
        /// <param name="e"></param>
        /// <param name="payload"></param>
        private void serverMessages(string s, Exception e, object payload)
        {
            if (s != null)
            {
                    //connected server message parsing
                    //When Communicator receives a connected command from the server,
                    //It starts to populate the client's spreadsheet.
                    //It uses a secondary callback named cellPopulationInit to complete spreadsheet cell population
                if (s.StartsWith("connected") && s.Length > 10)
                {
                    string temp = s.Substring(10);
                    remainingCellsForInit = int.Parse(temp);

                    if (remainingCellsForInit > 0)
                    {
                        socket.BeginReceive(cellPopulationInit, payload);
                    }
                    else
                    {
                        socket.BeginReceive(serverMessages, payload);
                        ThreadPool.QueueUserWorkItem(fx => editable());
                    }
                }
                    //cell server message parsing
                else if (s.StartsWith("cell") && s.Length > 5)
                {
                    string cellName;
                    string cellContents;
                    string temp = s.Substring(5);

                    cellName = temp.Substring(0, temp.IndexOf(' '));
                    cellContents = temp.Substring(temp.IndexOf(' ') + 1);

                        //callback to tell client to edit a cell
                    ThreadPool.QueueUserWorkItem(fx => updateCell(cellName, cellContents));
                    socket.BeginReceive(serverMessages, payload);
                }
                    //error server message parsing
                else if (s.StartsWith("error") && s.Length > 6)
                {
                        //Hands off error message parsing to method handleErrorMessage
                    handleErrorMessage(s);
                    socket.BeginReceive(serverMessages, payload);
                }
            }
        }

        /// <summary>
        /// Error message handling for server messages. Will parse server error messages and send
        /// ErrorID and Error summary to client.
        /// 
        /// Error ID Explanation:
        /// 
        /// Error ID 0: Generic or otherwise unforeseen error.
        /// Error ID 1: Invalid cell change request.
        /// Error ID 2: Command invalid or syntax unrecognized.
        /// Error ID 3: Unable to perform command in current state.
        /// Error ID 4: The username provided is invalid (in the case of
        ///             ‘connect’) or already in use (in the case of
        ///             registering a new user)
        /// </summary>
        /// <param name="s"></param>
        private void handleErrorMessage(string s)
        {
            string temp = s.Substring(6);
            int errorID = int.Parse(temp.Substring(0, temp.IndexOf(' ')));
            temp = temp.Substring(temp.IndexOf(' ') + 1);

            ThreadPool.QueueUserWorkItem(fx => error(errorID, temp));
        }

        /// <summary>
        /// Handles initial cell population.
        /// It sends the isEditable callback to the client once all cells have been received from the server.
        /// It populates the clients spreadsheet by the UpdateCell callback.
        /// </summary>
        /// <param name="s"></param>
        /// <param name="e"></param>
        /// <param name="payload"></param>
        private void cellPopulationInit(string s, Exception e, object payload)
        {
            if (s != null && s.Length > 5)
            {
                if (s.StartsWith("cell"))
                {
                    //parsing message
                    string cellName;
                    string cellContents;
                    string temp = s.Substring(5);

                    cellName = temp.Substring(0, temp.IndexOf(' '));
                    cellContents = temp.Substring(temp.IndexOf(' ') + 1);

                    ThreadPool.QueueUserWorkItem(fx => updateCell(cellName, cellContents));

                    //Decrements number of remaining cells to keep track of how many incoming cell messages are remaining.
                    remainingCellsForInit--;
                    if (remainingCellsForInit > 0)
                    {
                        socket.BeginReceive(cellPopulationInit, payload);
                    }
                    else
                    {
                        socket.BeginReceive(serverMessages, payload);
                        ThreadPool.QueueUserWorkItem(fx => editable());
                    }
                }
                //handles error message and keeps listening for cell messages if remainingCellsForInit > 0
                else if (s.StartsWith("error"))
                {
                    handleErrorMessage(s);

                    if (remainingCellsForInit > 0)
                    {
                        socket.BeginReceive(cellPopulationInit, payload);
                    }
                    else
                    {
                        socket.BeginReceive(serverMessages, payload);
                    }
                }
            }
        }

        /// <summary>
        /// Requests a Connection from the server to the specified spreadsheet using the specified username.
        /// port and serverIP are the port and server IP address to connect to the server. e is the type of encoding
        /// that will be sent by the client.
        /// </summary>
        /// <param name="userName"></param>
        /// <param name="spreadsheetName"></param>
        public void connect(string userName, string spreadsheetName, int port, IPAddress serverIP, Encoding e)
        {
            //Disconnect the socket if it is not null.
            if (socket != null)
            socket.Close();

            //Connecting client to server and establishing a socket
            client = new TcpClient();
            client.Connect(serverIP, port);
            socket = new StringSocket(client.Client, e);

            //Start receiving server messages
            socket.BeginSend("connect " + userName + " " + spreadsheetName + "\n", (ee, pp) => { }, payload);
            socket.BeginReceive(serverMessages, payload);
        }

        /// <summary>
        /// Requests that the server registers specified new user to user registry.
        /// </summary>
        /// <param name="clientName"></param>
        public void registerUser(string clientName)
        {
            socket.BeginSend("register " + clientName + "\n", (ee, pp) => { }, null);
        }

        /// <summary>
        /// Registers userName in server. Guarantees client username registration. Used if register fails.
        /// </summary>
        /// <param name="userName"></param>
        /// <param name="port"></param>
        /// <param name="serverIP"></param>
        /// <param name="e"></param>
        public static void registerUserAsAdmin(string userName, string spreadsheet, int port, IPAddress serverIP, Encoding e)
        {
            //Connecting client to server and establishing a socket
            TcpClient client = new TcpClient();
            client.Connect(serverIP, port);
            StringSocket socket = new StringSocket(client.Client, e);

            //Register userName using sysadmin workaround
            socket.BeginSend("connect sysadmin " + spreadsheet + "\n", (ee, pp) => { }, null);
            socket.BeginSend("register " + userName + "\n", (ee, pp) => { }, null);
            socket.Close();
            socket = null;
        }

        /// <summary>
        /// Requests that the specified cell be edited with the specified contents.
        /// </summary>
        /// <param name="cellName"></param>
        /// <param name="cellContents"></param>
        public void editCell(string cellName, string cellContents)
        {
            socket.BeginSend("cell " + cellName + " " + cellContents + "\n", (ee, pp) => { }, null);
        }

        /// <summary>
        /// Requests that the last edit be undone.
        /// NOTE: It can undo other users edits.
        /// </summary>
        public void undoCell()
        {
            socket.BeginSend("undo\n", (ee, pp) => { }, null);
        }

        /// <summary>
        /// Closes StringSocket connection to server. Does not remove Delegates.
        /// </summary>
        public void closeConnection()
        {
            socket.Close();
            socket = null;
        }
    }
}
