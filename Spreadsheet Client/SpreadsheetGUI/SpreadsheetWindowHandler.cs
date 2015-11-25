using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;
using SS;
using SpreadsheetGUI;


namespace SS
{
    /// <summary>
    /// Keeps track of how many top-level forms are running
    /// </summary>
    class SpreadsheetWindowHandler : ApplicationContext
    {
        // Number of open forms
        private int formCount = 0;

        // Singleton ApplicationContext
        private static SpreadsheetWindowHandler appContext;
        /// <summary>
        /// Private constructor for singleton pattern
        /// </summary>
        private SpreadsheetWindowHandler()
        {
        }

        /// <summary>
        /// Returns the one DemoApplicationContext.
        /// </summary>
        public static SpreadsheetWindowHandler getAppContext()
        {
            if (appContext == null)
            {
                appContext = new SpreadsheetWindowHandler();
            }
            return appContext;
        }

        /// <summary>
        /// Runs the form
        /// </summary>
        public bool RunForm(Form form)
        {
            // One more form is running
            formCount++;

            // When this form closes, we want to find out
            form.FormClosed += (o, e) => { if (--formCount <= 0) ExitThread(); };

            // Run the form
            if (!form.IsDisposed)
            {
                form.Show();
                return true;
            }
            else
            {
                return false;
            }

        }
    }


    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            // Start an application context and run one form inside it
            SpreadsheetWindowHandler appContext = SpreadsheetWindowHandler.getAppContext();
            if (appContext.RunForm(new GUI()))
                Application.Run(appContext);
        }
    }

}

