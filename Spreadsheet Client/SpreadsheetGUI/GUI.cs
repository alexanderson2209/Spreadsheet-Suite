/*
  File: GUI.cs
  Author: Alexander Anderson
  Team: SegFault
  CS 3505 - Spring 2015
  Date created: March 30, 2015
  Last updated: April 20, 2015
   
   
  Resources:
  - Spreadsheet                     from CS3500 Fall 2014
  - Communicator                    by Alexander Anderson Spring 2015
  - Formula.dll                     by Alexander Anderson Fall 2014
  - DependencyGraph.dll             by Alexander Anderson Fall 2014
  - Spreadsheet Panel               from CS3500 Fall 2014
  - SpreadsheetWindowHandler.cs     from CS3500 Fall 2014
*/
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using SpreadsheetUtilities;
using SS;
using System.Diagnostics;
using ClientServerCommunications;
using System.Net;
using System.Threading;
using System.Net.Sockets;

namespace SpreadsheetGUI
{
    public partial class GUI : Form
    {
        //loading screen needs global scope because multiple methods require hide/show capability for this field
        private LoadingScreen loading = new LoadingScreen();
        private delegate void RedXFixer(int col, int row, string cell);
        private Communicator comm;
        private delegate void hideLoadingScreen();
        private bool editable;
        private LogIn logInScreen;
        private object CellEditLock = new object();
        /// <summary>
        /// Underlying spreadsheet handler. Handles formulas, cell names, contents, values, and their dependencies.
        /// </summary>
        private Spreadsheet ss = new Spreadsheet(s => true, s => s.ToUpper(), "ps6");

        /// <summary>
        /// GUI aspect of the spreadsheet. Spreadsheetpanel is the panel that controls the actual spreadsheet. This part controls
        /// the other aspects (saving, error handling etc).
        /// </summary>
        public GUI()
        {
            InitializeComponent();

            //log in screen for spreadsheet
            logInScreen = new LogIn();
            logInScreen.ShowDialog(this);

            //If logInScreen has a DialogResult of Cancel, then close the application.
            if (logInScreen.DialogResult == DialogResult.Cancel) this.Close();

            //Checks for valid host address/port
            //If invalid, shows logInScreen again.
            bool connected = false;
            while (!connected && logInScreen.DialogResult == DialogResult.OK)
            {
                try
                {
                    //Server Connection starts here.
                    IPAddress[] ips = Dns.GetHostAddresses(logInScreen.getServer());
                    comm = new Communicator(UpdateCell, ErrorCB, isEditable);
                    comm.connect(logInScreen.getUsername(), logInScreen.getSpreadsheetName(), logInScreen.getPort(), ips[0], Encoding.ASCII);

                    connected = true;
                    this.Text = logInScreen.getSpreadsheetName();

                    /*
                     * In order to Make LoadingScreen work while hiding the spreadsheet window, The loadingscreen
                     * is shown in a seperate thread and The main thread is haulted until the Editable callback is triggered.
                     * When the editable callback is triggered, It flips the bool variable editable, causing the main thread to finish
                     * executing.
                     */
                    ThreadPool.QueueUserWorkItem(fx => loading.ShowDialog());

                    while (!editable)
                    {
                        Thread.Sleep(100);
                    }
                    hideLoadingScreen hls = loading.close;
                    loading.Invoke(hls);
                }
                catch (SocketException e)
                {
                    MessageBox.Show("Cannot connect to server. Check server address and port.");
                    logInScreen.ShowDialog();
                }
            }
        }

        /// <summary>
        /// Hides loadingScreen and shows spreadhseet. Used in Communicator as a Callback for making sure a client cant edit
        /// a spreadsheet during initial population.
        /// </summary>
        private void isEditable()
        {
            lock (ss)
            {
                //When all cells have finished loading, this will redo all cells to eliminate any FormulaErrors
                //caused by calculating cell values before all cells have been loaded.

                redoAllAffectedCells(new HashSet<string>(ss.GetNamesOfAllNonemptyCells()));

                //Flips boolean to enable main thread's execution.
                editable = true;
            }
        }

        /// <summary>
        /// Used by Communicator as a callback for error reporting.
        /// Info is used to convey extra information about a certain error.
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
        /// <param name="errorID"></param>
        /// <param name="info"></param>
        private void ErrorCB(int errorID, string info)
        {
            switch (errorID)
            {
                case 0: MessageBox.Show("Error 0: Generic or otherwise unforseen error. " + info,
                            "Error 0", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    break;
                case 1: MessageBox.Show("Error 1: Invalid cell change request. " + info,
                            "Error 1", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    break;
                case 2: MessageBox.Show("Error 2: Command invalid or syntax unrecognized. " + info,
                            "Error 2", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    break;
                case 3: MessageBox.Show("Error 3: Unable to perform command in current state. " + info,
                            "Error 3", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    break;
                case 4: 
                        //Calls Communicator's static method registerUserAsAdmin and then reconnects as non-admin
                        Communicator.registerUserAsAdmin(logInScreen.getUsername(), logInScreen.getSpreadsheetName(), logInScreen.getPort(),
                            Dns.GetHostAddresses(logInScreen.getServer())[0], Encoding.ASCII);
                        comm.connect(logInScreen.getUsername(), logInScreen.getSpreadsheetName(), logInScreen.getPort(),
                            Dns.GetHostAddresses(logInScreen.getServer())[0], Encoding.ASCII);
                        break;
            }
        }

        /// <summary>
        /// Used by Communicator as a callback for Cell edit confirmation.
        /// Updates specified cell with specified contents.
        /// It updates all affected cells as well.
        /// </summary>
        /// <param name="cell"></param>
        /// <param name="contents"></param>
        private void UpdateCell(string cell, string contents)
        {
            lock (ss)
            {
                HashSet<string> needToRecalc = (HashSet<string>)ss.SetContentsOfCell(cell, contents);
                redoAllAffectedCells(needToRecalc);
            }
        }

        /// <summary>
        /// Event handler for "Enter" key being pressed on the cellContents text box. It sends a cell message to the server requesting
        /// a cell to be changed.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cellContents_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (e.KeyChar == (char)Keys.Return)
            {
                int col;
                int row;
                spreadsheetPanel1.GetSelection(out col, out row);

                //converting the zero-based cell location to the Alphanumeric representation.
                char colChar = (char)(col + 65);
                string cell = colChar.ToString() + (row + 1);
                string contents = cellContents.Text;
                try
                {
                    if (cellContents.Text.Length > 0 && cellContents.Text.ElementAt(0) == '=')
                    {
                        //check to see if it can make a new formula. This will check for format errors before sending to server.
                        Formula temporaryFormula = new Formula(cellContents.Text.Substring(1), ss.Normalize, ss.IsValid);
                        contents = "=" + temporaryFormula.ToString();
                    }
                    //sends cell name and cell contents to server for synchronization. 
                    comm.editCell(cell, contents);
                }
                catch (FormulaFormatException ex)
                {
                    System.Windows.Forms.MessageBox.Show(cell + " has an incorrect formula.");
                }
            }

        }

        /// <summary>
        /// Determines the type of the value stored in specified cell and sets the spreadsheetpanel's appropriate cell value.
        /// Value can either be a double, string, or a formula error.
        /// Used as a private helper method in method redoAllAffectedCells.
        /// </summary>
        /// <param name="col"></param>
        /// <param name="row"></param>
        /// <param name="cell"></param>
        private void setPanelValue(int col, int row, string cell)
        {
            lock (ss)
            {
                object ssValue = ss.GetCellValue(cell);

                //Make sure other threads dont set the spreadsheet value at the same time.
                lock (CellEditLock)
                {
                    if (ssValue is double)
                    {
                        spreadsheetPanel1.SetValue(col, row, ((double)ssValue).ToString());
                    }
                    else if (ssValue is string)
                    {
                        spreadsheetPanel1.SetValue(col, row, (string)ssValue);
                    }
                    else
                    {
                        spreadsheetPanel1.SetValue(col, row, "error");
                    }
                }
            }
        }
        /// <summary>
        /// Sets all values specified in AffectedCells to the correct value in the spreadsheetpanel.
        /// </summary>
        /// <param name="form"></param>
        /// <param name="AffectedCells"></param>
        private void redoAllAffectedCells(ISet<string> AffectedCells)
        {
            foreach (string s in AffectedCells)
            {
                int col = ((int)s.ElementAt(0) - 65);
                int row = int.Parse(s.Substring(1)) - 1;
                setPanelValue(col, row, s);
            }
        }
        /// <summary>
        /// Changes the cell name, value, and contents according to the cell you select.
        /// </summary>
        /// <param name="sender"></param>
        private void spreadsheetPanel1_SelectionChanged(SpreadsheetPanel sender)
        {
            //getting cell name from col and row of spreadsheetpanel
            int col;
            int row;
            spreadsheetPanel1.GetSelection(out col, out row);
            char colChar = (char)(col + 65);
            string cell = colChar.ToString() + (row + 1);

            //setting cellName text to the name of the cell
            cellName.Text = cell;

            //setting cellContents text to the contents of the selected cell.
            lock (ss)
            {
                object tempCellContents = ss.GetCellContents(cell);
                if (tempCellContents is Formula)
                {
                    cellContents.Text = "=" + ((Formula)tempCellContents).ToString();
                }
                else
                {
                    cellContents.Text = tempCellContents.ToString();
                }
                cellContents.Select(cellContents.Text.Length, 0);
            }

            //sets cellValue text after setting cell to new value.
            string tempValueOut;
            spreadsheetPanel1.GetValue(col, row, out tempValueOut);
            cellValue.Text = tempValueOut;
        }

        /// <summary>
        /// Event handler for open button on menu bar. Opens spreadsheet in a new window.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void openToolStripMenuItem_Click(object sender, EventArgs e)
        {
            // Tell the application context to run the form on the same
            // thread as the other forms.
            GUI temp = new GUI();

            //if GUI hasnt been closed in the Ctor
            if (!temp.IsDisposed)
                SpreadsheetWindowHandler.getAppContext().RunForm(temp);
        }

        /// <summary>
        /// Event Handler for closing the spreadsheet. Has yes/no confirmation.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void GUI_FormClosing(object sender, FormClosingEventArgs e)
        {
            //If LogInScreen's DialogResult is cancel, We dont need to check if its ok to exit. Just exit.
            if (logInScreen.DialogResult != DialogResult.Cancel)
            {
                string messageBoxText = "Are you sure you want to exit?";
                string caption = "Spreadsheet";
                MessageBoxButtons button = MessageBoxButtons.YesNo;
                MessageBoxIcon icon = MessageBoxIcon.Warning;

                DialogResult dr = MessageBox.Show(messageBoxText, caption, button, icon);
                if (dr == DialogResult.No) e.Cancel = true;
            }
        }

        /// <summary>
        /// Information Event. Shows Information.txt in resource folder.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void informationToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Process.Start(@"..\..\..\Resources\Resources\Information.txt");
        }

        /// <summary>
        /// Requests that the server undo the last successful cell change.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void undoButton_Click(object sender, EventArgs e)
        {
            comm.undoCell();
        }

        /// <summary>
        /// Closes current spreadsheet. Will ask for confirmation.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void closeToolStripMenuItem_Click(object sender, EventArgs e)
        {
            this.Close();
        }
    }
}
