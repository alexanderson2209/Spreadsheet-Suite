/*
  File: LogIn.cs
  Author: Alexander Anderson
  Team: SegFault
  CS 3505 - Spring 2015
  Date created: March 30, 2015
  Last updated: April 20, 2015
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

namespace SpreadsheetGUI
{
    public partial class LogIn : Form
    {
        /// <summary>
        /// RedXExit is used to determine if LogIn Form was closed by top right red x.
        /// </summary>
        private bool RedXExit = true;
        public LogIn()
        {
            InitializeComponent();
        }

        /// <summary>
        /// LogIn button click Event. Checks for valid user input syntax.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void logInButton_Click(object sender, EventArgs e)
        {
            if (userNameTextBox.TextLength > 0 && spreadsheetTextBox.TextLength > 0 
                && serverTextBox.TextLength > 0 && portTextBox.TextLength > 0)
            {
                int portCheck;
                //Checks for invalid user/spreadsheet name.
                if (userNameTextBox.Text.Contains('\n') || userNameTextBox.Text.Contains(' ') || spreadsheetTextBox.Text.Contains('\n'))
                {
                    MessageBox.Show("Usernames must not contain spaces or newline " +
                       "characters and spreadsheet names must not contain newline characters.",
                       "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
                //Checks for invalid port number.
                else if (!int.TryParse(portTextBox.Text, out portCheck))
                {
                    MessageBox.Show("Port must be an integer.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
                //Checks for invalid host name.
                else if (Uri.CheckHostName(serverTextBox.Text) == UriHostNameType.Unknown)
                {
                    MessageBox.Show("Invalid host address. Check Server textbox for errors.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
                else
                {
                    RedXExit = false;
                    this.DialogResult = DialogResult.OK;
                }
            }
            else
            {
                MessageBox.Show("Must have a Username, Spreadsheet name, server, and port specified before logging in.",
                    "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        /// <summary>
        /// Elegantly closes LogIn window if the top right red x is pressed.
        /// </summary>
        /// <param name="e"></param>
        protected override void OnFormClosing(FormClosingEventArgs e)
        {
            if (RedXExit) DialogResult = DialogResult.Cancel;
        }

        /// <summary>
        /// Gets text from Username text box.
        /// </summary>
        /// <returns></returns>
        public string getUsername()
        {
            return userNameTextBox.Text;
        }

        /// <summary>
        /// Gets text from Spreadsheet text box.
        /// </summary>
        /// <returns></returns>
        public string getSpreadsheetName()
        {
            return spreadsheetTextBox.Text;
        }

        /// <summary>
        /// Gets text from Port text box.
        /// </summary>
        /// <returns></returns>
        public int getPort()
        {
            int tempParseInt;
            int.TryParse(portTextBox.Text, out tempParseInt);
            return tempParseInt;
        }

        /// <summary>
        /// Gets server from server text box.
        /// </summary>
        /// <returns></returns>
        public string getServer()
        {
            return serverTextBox.Text;
        }

        /// <summary>
        /// Cancel button Event. Closes out of Application.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cancelButton_Click(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
        }

        /// <summary>
        /// Button press event for LogIn text boxes.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void textBox_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (e.KeyChar == (char)Keys.Return)
            {
                logInButton_Click(sender, e);
            }
            else if (e.KeyChar == (char)Keys.Escape)
            {
                DialogResult = DialogResult.Cancel;
            }
        }
    }
}
