/*
  File: LoadingScreen.cs
  Author: Alexander Anderson
  Team: SegFault
  CS 3505 - Spring 2015
  Date created: March 30, 2015
  Last updated: April 20, 2015
   
   
  Resources:
  - 299.GIF                         from Preloaders.net
  - 712.GIF                         from Preloaders.net
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
    public partial class LoadingScreen : Form
    {
        public LoadingScreen()
        {
            InitializeComponent();
        }

        public void close()
        {
            DialogResult = DialogResult.OK;
        }
    }
}
