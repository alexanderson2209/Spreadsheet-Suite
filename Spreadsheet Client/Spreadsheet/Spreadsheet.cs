/*
  File: Spreadsheet.cs
  Author: Alexander Anderson
  Team: SegFault
  CS 3505 - Spring 2015
  Date created: March 30, 2015
  Last updated: April 20, 2015
   
   
  Resources:
  - Formula.dll                     by Alexander Anderson Fall 2014
  - DependencyGraph.dll             by Alexander Anderson Fall 2014
  - AbstractSpreadsheet.cs          by Joe Zachary for CS 3500, September 2012
*/

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using SpreadsheetUtilities;
using System.Text.RegularExpressions;
using System.Xml;
using System.IO;
using System.Data.OleDb;
using System.Data;

namespace SS
{
    /// <summary>
    /// Basic framework for a spreadsheet utility. This class handles cells and their contents and values.
    /// It also handles formulas, doubles, and text in cells.
    /// It handles formulas with variables that are other cells. (cell dependency)
    /// Will be used for spreadsheet GUI.
    /// </summary>
    public class Spreadsheet : AbstractSpreadsheet
    {
        /// <summary>
        /// cells keeps track of the cell name to cell instance relationship.
        /// If a cell is named "A1", it will point to the cell instance of "A1".
        /// </summary>
        private Dictionary<string, Cell> cells;

        /// <summary>
        /// Keeps track of the dependees and dependents of a cell.
        /// For example, if cell "A1" has a formula "B1+3", A1 would be dependent on B1, and B1 would be a dependee of A1.
        /// </summary>
        private DependencyGraph dependencies;

        /// <summary>
        /// Changed private variable.
        /// </summary>
        private bool _Changed;

        /// <summary>
        /// Provides a default constructor for Spreadsheet.
        /// 
        /// Creates a new Spreadsheet with default isValid, normalize, and version values.
        /// 
        /// isValid parameter checks for cell name validity.
        /// normalize parameter changes cell names to fit specification
        /// version will ensure the correct file is loaded from disk.
        /// 
        /// isValid is default initialized with s => true
        /// normalize is default initialized with s => s
        /// version is default initialized with "default"
        /// </summary>
        public Spreadsheet()
            : base(s => true, s => s, "default")
        {
            cells = new Dictionary<string, Cell>();
            dependencies = new DependencyGraph();
            Changed = false;
        }

        /// <summary>
        /// Creates a new Spreadsheet with parameter specified isValid, normalize, and version values.
        /// 
        /// isValid parameter checks for cell name validity.
        /// normalize parameter changes cell names to fit specification
        /// version will ensure the correct file is loaded from disk.
        /// </summary>
        /// <param name="isValid"></param>
        /// <param name="normalize"></param>
        /// <param name="version"></param>
        public Spreadsheet(Func<string, bool> isValid, Func<string, string> normalize, string version)
            : base(isValid, normalize, version)
        {
            cells = new Dictionary<string, Cell>();
            dependencies = new DependencyGraph();
            Changed = false;
        }

        /// <summary>
        /// Reads in a Spreadsheet from the specified filePath. The file should be an XML file.
        /// 
        /// isValid parameter checks for cell name validity.
        /// normalize parameter changes cell names to fit specification
        /// version will ensure the correct file is loaded from disk.
        /// 
        /// If anything goes wrong while reading the file, a SpreadsheetReadWriteException should be thrown.
        /// If version mismatch occurs, it will throw an exception.
        /// If an incorrect cell name is come across, an exception will be thrown.
        /// If an incorrect/missing cell content is come across, an exception will be thrown.
        /// </summary>
        /// <param name="filePath"></param>
        /// <param name="isValid"></param>
        /// <param name="normalize"></param>
        /// <param name="version"></param>
        public Spreadsheet(string filePath, Func<string, bool> isValid, Func<string, string> normalize, string version)
            : base(isValid, normalize, version)
        {
            cells = new Dictionary<string, Cell>();
            dependencies = new DependencyGraph();

            ReadSpreadsheet(filePath, version);

            Changed = false;
        }

        /// <summary>
        /// 3 Arg Ctor for Spreadsheet. Used to import Excel XML spreadsheet.
        /// </summary>
        /// <param name="filePath"></param>
        /// <param name="isValid"></param>
        /// <param name="normalize"></param>
        public Spreadsheet(string filePath, Func<string, bool> isValid, Func<string, string> normalize)
            : base(isValid, normalize, "ps6")
        {
            cells = new Dictionary<string, Cell>();
            dependencies = new DependencyGraph();
            ExcelReader(filePath);

            Changed = false;
        }

        /// <summary>
        /// Reads in a specified file to spreadsheet.
        /// </summary>
        /// <param name="filename"></param>
        /// <param name="version"></param>
        private void ReadSpreadsheet(string filename, string version)
        {
            //reading in the XML file
            XmlReader reader = null;
            try
            {
                reader = XmlReader.Create(filename);

                //name and contents of the cell
                string name = null;
                string contents = null;
                while (reader.Read())
                {

                    if (reader.IsStartElement())
                    {
                        //checks whether the element is spreadsheet, name, or contents and handles it appropriately.
                        switch (reader.Name)
                        {
                            //if versions dont match, exception is thrown.
                            case "spreadsheet":
                                if (!version.Equals(reader["version"]))
                                    throw new SpreadsheetReadWriteException("Version mismatch");
                                break;

                            case "cell":

                                if (name != null && contents == null)
                                    throw new SpreadsheetReadWriteException("Contents are missing from the cell");

                                //resetting nulls for "missing name or contents" checking.
                                name = null;
                                contents = null;
                                break;

                            //reads in name element
                            case "name":
                                reader.Read();
                                name = reader.Value;
                                break;

                            //reads in contents element. If name is invalid or contents is invalid/missing, exceptions are thrown.
                            case "contents":
                                reader.Read();
                                contents = reader.Value;
                                SetContentsOfCell(name, contents);

                                break;
                            //if element isnt spreadsheet, cell, name, or contents. Throws exception
                            default:
                                throw new SpreadsheetReadWriteException(reader.Name + " is an Illegal Element in this XML.");
                        }
                    }
                }
            }
            catch (FileNotFoundException e)
            {
                throw new SpreadsheetReadWriteException("\"" + filename + "\"" + " does not exist.");
            }
            catch (DirectoryNotFoundException e)
            {
                throw new SpreadsheetReadWriteException("\"" + filename + "\"" + ". Directory doesnt exist.");

            }

            //catches any XmlException and throws a SpreadsheetReadWriteException, propagating the original exception's error message.
            catch (XmlException e)
            {
                throw new SpreadsheetReadWriteException(e.GetBaseException().Message);
            }
            catch (InvalidNameException e)
            {
                throw new SpreadsheetReadWriteException("An invalid cell name was used. File corruption might have occured.");
            }
            finally
            {
                if (reader != null)
                    ((IDisposable)reader).Dispose();
            }
        }

        /// <summary>
        /// Returns an IEnumberable of type string that contains the names of all non-empty cells.
        /// A cell that contains "" in its contents field is considered empty.
        /// </summary>
        /// <returns></returns>
        public override IEnumerable<string> GetNamesOfAllNonemptyCells()
        {
            List<string> names = new List<string>();
            foreach (KeyValuePair<string, Cell> el in cells.AsEnumerable())
            {
                //checks if Cell is non-empty. If contents is string and it == "", it is considered empty and not included.
                    if (el.Key != null && (!(el.Value.contents is string) || ((string)el.Value.contents != "")))
                        names.Add(el.Value.name);
            }
            return names;
        }

        /// <summary>
        /// If name is null or invalid, throws an InvalidNameException.
        /// 
        /// Otherwise, returns the contents of the named cell. The return
        /// value should be either a string, a double, or a Formula.
        /// 
        /// If the cell is empty, it returns "".
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        public override object GetCellContents(string name)
        {
            //checking if name == null and normalizing name if it isnt null.
            if (name == null) throw new InvalidNameException();
            name = Normalize(name);

            //if name is not valid, throws InvalidNameException
            if (!isValidVariable(name)) throw new InvalidNameException();

            //if cell doesnt exist, return "".
            if (!cells.ContainsKey(name)) return "";

            //otherwise, return contents.
            return cells[name].contents;
        }

        /// <summary>
        /// If name is invalid or null, throws InvalidNameException.
        /// contents of the named cell become a double.
        /// 
        /// The method returns a set consisting of name plus the names of all other cells whose value depends, 
        /// directly or indirectly, on the named cell.
        /// 
        /// For example, if name is A1, B1 contains A1*2, and C1 contains B1+A1, the
        /// set {A1, B1, C1} is returned.
        /// </summary>
        /// <param name="name"></param>
        /// <param name="number"></param>
        /// <returns></returns>
        protected override ISet<string> SetCellContents(string name, double number)
        {
            //adds new cell "name". Parsing double to correct formatting. 5.0000 == 5.0
            cells[name] = new Cell(name, number, double.Parse(number.ToString()));
            Changed = true;

            //fixes all dependees of name
            recalculateDependees(name, (LinkedList<string>)GetCellsToRecalculate(name));

            //removes all dependents of name since name isnt a formula
            foreach (string el in dependencies.GetDependents(name))
            {
                dependencies.RemoveDependency(name, el);
            }

            //grabs name and dependees(direct and indirect) of name, puts them into a HashSet and returns them.
            return new HashSet<string>(GetCellsToRecalculate(name));
        }

        /// <summary>
        /// If name is invalid or null, throws InvalidNameException.
        /// contents of the named cell become a string.
        /// 
        /// The method returns a set consisting of name plus the names of all other cells whose value depends, 
        /// directly or indirectly, on the named cell.
        /// 
        /// For example, if name is A1, B1 contains A1*2, and C1 contains B1+A1, the
        /// set {A1, B1, C1} is returned.
        /// </summary>
        /// <param name="name"></param>
        /// <param name="text"></param>
        /// <returns></returns>
        protected override ISet<string> SetCellContents(string name, string text)
        {
            //Replacing cell name with new content and value.
            cells[name] = new Cell(name, text, text);

            //spreadsheet has changed
            Changed = true;

            //recalculates all dependees of name.
            recalculateDependees(name, (LinkedList<string>)GetCellsToRecalculate(name));

            //remove all dependencies of name, since name is now a text cell and not a Formula.
            foreach (string el in dependencies.GetDependents(name))
            {
                dependencies.RemoveDependency(name, el);
            }

            //grabs name and dependees(indirect and direct) of name, puts them into a HashSet and returns them.
            return new HashSet<string>(GetCellsToRecalculate(name));
        }

        /// <summary>
        /// If the formula parameter is null, throws an ArgumentNullException.
        /// 
        /// Otherwise, if name is null or invalid, throws an InvalidNameException.
        /// 
        /// contents of the named cell become a formula.
        /// 
        /// The method returns a set consisting of name plus the names of all other cells whose value depends, 
        /// directly or indirectly, on the named cell.
        /// 
        /// For example, if name is A1, B1 contains A1*2, and C1 contains B1+A1, the
        /// set {A1, B1, C1} is returned.
        /// </summary>
        /// <param name="name"></param>
        /// <param name="formula"></param>
        /// <returns></returns>
        protected override ISet<string> SetCellContents(string name, Formula formula)
        {

            LinkedList<string> cellsToRecalc;
            cellsToRecalc = (LinkedList<string>)GetCellsToRecalculate(name);

            //Testing for CircularExceptions, does not affect cells so no need to backup old cell.
            foreach (string el in cellsToRecalc)
            {
                if (formula.GetVariables().Contains(el))
                    throw new CircularException();
            }

            //Replacing cell name with new content and value.
            try
            {
                cells[name] = new Cell(name, formula, formula.Evaluate(lookup));
            }
            catch (System.ArgumentException e)
            {
                cells[name] = new Cell(name, formula, new FormulaError());
            }

            //spreadsheet has changed
            Changed = true;

            //recalculates the dependees of name.
            recalculateDependees(name, cellsToRecalc);

            //replace all dependents of name with new formulas variables.
            dependencies.ReplaceDependents(name, formula.GetVariables());

            //grabs name and dependees(direct and indirect) of name and puts them into a HashSet and returns them.
            return new HashSet<string>(cellsToRecalc);
        }

        /// <summary>
        /// Fixes all dependee values and recalculates them. It will also fix FormulaErrors if they have been resolved or create new FormulaErrors
        /// if the cell's contents were changed to the wrong type.
        /// </summary>
        /// <param name="s"></param>
        /// <param name="cellsToRecalc"></param>
        private void recalculateDependees(string s, LinkedList<string> cellsToRecalc)
        {
            //for each cell el, its going to recalculate the value of the cell.
            foreach (string el in cellsToRecalc)
            {
                //if cell el is a Formula, its going to recalculate that formulas values.
                //this if statement is nessecary because the first element in cellsToRecalc is the variable s, which has been calculated already.
                if (el == s)
                {
                    //do nothing.
                }
                else
                {
                    Cell temporaryCell = cells[el];
                    //Replacing cell name with new value. If Evaluate fails due to invalid lookup,
                    //the value of the cell gets a FormulaError.
                    temporaryCell.value = ((Formula)temporaryCell.contents).Evaluate(lookup);
                }
            }
        }

        /// <summary>
        /// Lookup function for Formulas evaluate method. It throws new ArgumentException if s's value isnt a double or if it doesnt exist.
        /// It throws an ArgumentException because thats how the Formula class wants it.
        /// </summary>
        /// <param name="s"></param>
        /// <returns></returns>
        private double lookup(string s)
        {
            if (!cells.ContainsKey(s))
            {
                throw new System.ArgumentException();
            }
            else
            {
                Object temp = cells[s].value;
                //if the cell doesnt exist or the cells value is not a double, it errors.
                if (temp is double)
                {
                    return (double)temp;
                }
                else throw new ArgumentException();
            }
        }

        /// <summary>
        /// checks if variable is valid or not. If valid, returns true. Otherwise, returns false.
        /// 
        /// A string is a valid cell name if and only if:
        /// (1) its first character is an underscore or a letter
        /// (2) its remaining characters (if any) are underscores and/or letters and/or digits
        /// </summary>
        /// <param name="s"></param>
        /// <returns></returns>
        private bool isValidVariable(string s)
        {
            String varPattern = @"^[a-zA-Z]+[0-9]+$";

            if (Regex.IsMatch(s, varPattern) && IsValid(s)) return true;
            else return false;
        }

        /// <summary>
        /// returns direct dependents of name.
        /// If name is null. Throw ArgumentNullException()
        /// If name isnt a valid cell name. Throw an InvalidNameException.
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        protected override IEnumerable<string> GetDirectDependents(string name)
        {
            //if name parameter is null, throw ArgumentNullException
            if (name == null) throw new ArgumentNullException();

            name = Normalize(name);

            //if name isnt a valid variable name, throw InvalidNameException
            if (!isValidVariable(name)) throw new InvalidNameException();

            //returns direct dependents of name.
            return dependencies.GetDependees(name);
        }

        /// <summary>
        /// Returns the version information of the spreadsheet saved in the named file.
        /// If there are any problems opening, reading, or closing the file, the method
        /// should throw a SpreadsheetReadWriteException with an explanatory message.
        /// </summary>
        public override string GetSavedVersion(string filename)
        {
            XmlReader reader = null;
            try
            {
                //reads specified filename
                reader = XmlReader.Create(filename);

                while (reader.Read())
                {
                    //finds spreadsheed element and reads its version attribute
                    if (reader.Name == "spreadsheet")
                    {
                        if (reader["version"] == null) throw new SpreadsheetReadWriteException("Version Does not exist.");
                        return reader["version"];
                    }
                }
                //if spreadsheet element cannot be found and returned. Exception is thrown.
                throw new SpreadsheetReadWriteException("cannot find version number");
            }
            catch (FileNotFoundException e)
            {
                throw new SpreadsheetReadWriteException("\"" + filename + "\"" + " does not exist.");
            }

            //catches any XmlException and throws a SpreadsheetReadWriteException, propagating the original exception's error message.
            catch (XmlException e)
            {
                throw new SpreadsheetReadWriteException(e.GetBaseException().Message);
            }
            finally
            {
                if (reader != null)
                    ((IDisposable)reader).Dispose();
            }
        }

        /// <summary>
        /// Writes the contents of this spreadsheet to the named file using an XML format.
        /// The XML elements should be structured as follows:
        /// 
        /// <spreadsheet version="version information goes here">
        /// 
        /// <cell>
        /// <name>
        /// cell name goes here
        /// </name>
        /// <contents>
        /// cell contents goes here
        /// </contents>    
        /// </cell>
        /// 
        /// </spreadsheet>
        /// 
        /// There should be one cell element for each non-empty cell in the spreadsheet.  
        /// If the cell contains a string, it should be written as the contents.  
        /// If the cell contains a double d, d.ToString() should be written as the contents.  
        /// If the cell contains a Formula f, f.ToString() with "=" prepended should be written as the contents.
        /// 
        /// If there are any problems opening, writing, or closing the file, the method should throw a
        /// SpreadsheetReadWriteException with an explanatory message.
        /// </summary>
        public override void Save(string filename)
        {
            XmlWriter writer = null;
            try
            {
                writer = XmlWriter.Create(filename);

                writer.WriteStartElement("spreadsheet");
                writer.WriteAttributeString("version", Version);
                foreach (string el in GetNamesOfAllNonemptyCells())
                {
                    writer.WriteStartElement("cell");
                    writer.WriteElementString("name", el);

                    //checking whether the content is a string, double, or a formula.
                    //and writing the appropriate data.
                    Object contentsOfCell = GetCellContents(el);
                    if (contentsOfCell is string)
                    {
                        writer.WriteElementString("contents", (string)contentsOfCell);
                    }
                    else if (contentsOfCell is double)
                    {
                        writer.WriteElementString("contents", ((double)contentsOfCell).ToString());
                    }
                    else if (contentsOfCell is Formula)
                    {
                        writer.WriteElementString("contents", "=" + ((Formula)contentsOfCell).ToString());
                    }

                    writer.WriteEndElement(); //end cell
                }
                writer.WriteEndElement();  //end spreadsheet
            }
            catch (DirectoryNotFoundException e)
            {
                throw new SpreadsheetReadWriteException("\"" + filename + "\"" + ". Directory doesnt exist.");
            }
            //catches any XmlException and throws a SpreadsheetReadWriteException, propagating the original exception's error message.
            catch (XmlException e)
            {
                throw new SpreadsheetReadWriteException(e.GetBaseException().Message);
            }
            finally
            {
                if (writer != null)
                    ((IDisposable)writer).Dispose();
            }
            Changed = false;
        }

        /// <summary>
        /// If name is null or invalid, throws an InvalidNameException.
        /// 
        /// Otherwise, returns the value (as opposed to the contents) of the named cell.  The return
        /// value should be either a string, a double, or a SpreadsheetUtilities.FormulaError.
        /// </summary>
        public override object GetCellValue(string name)
        {

            //checking if name == null and normalizing name if it isnt null.
            if (name == null) throw new InvalidNameException();
            name = Normalize(name);

            //if name is not valid, throws InvalidNameException
            if (!isValidVariable(name)) throw new InvalidNameException();

            if (!cells.ContainsKey(name))
                return "";
            return cells[name].value;

        }

        /// <summary>
        /// If content is null, throws an ArgumentNullException.
        /// 
        /// Otherwise, if name is null or invalid, throws an InvalidNameException.
        /// 
        /// Otherwise, if content parses as a double, the contents of the named
        /// cell becomes that double.
        /// 
        /// Otherwise, if content begins with the character '=', an attempt is made
        /// to parse the remainder of content into a Formula f using the Formula
        /// constructor.  There are then three possibilities:
        /// 
        ///   (1) If the remainder of content cannot be parsed into a Formula, a 
        ///       SpreadsheetUtilities.FormulaFormatException is thrown.
        ///       
        ///   (2) Otherwise, if changing the contents of the named cell to be f
        ///       would cause a circular dependency, a CircularException is thrown.
        ///       
        ///   (3) Otherwise, the contents of the named cell becomes f.
        /// 
        /// Otherwise, the contents of the named cell becomes content.
        /// 
        /// If an exception is not thrown, the method returns a set consisting of
        /// name plus the names of all other cells whose value depends, directly
        /// or indirectly, on the named cell.
        /// 
        /// For example, if name is A1, B1 contains A1*2, and C1 contains B1+A1, the
        /// set {A1, B1, C1} is returned.
        /// </summary>
        public override ISet<string> SetContentsOfCell(string name, string content)
        {
            //if content is null, throws ArgumentNullException
            if (content == null) throw new ArgumentNullException();

            //checking if name == null and normalizing name if it isnt null.
            if (name == null) throw new InvalidNameException();
            name = Normalize(name);

            //if name is not valid, throws InvalidNameException
            if (!isValidVariable(name)) throw new InvalidNameException();

            //if content is double, parse double and add it using SetCellContents
            double doubleContent;
            if (double.TryParse(content, out doubleContent))
            {
                return SetCellContents(name, doubleContent);
            }
            //if content starts with "=", try to add it using whatever comes after = as a formula.
            else if (Regex.IsMatch(content, @"^=.*"))
            {
                content = content.Remove(0, 1);
                return SetCellContents(name, new Formula(content, Normalize, IsValid));
            }
            //if content is just a string, add it to cell.
            else
            {
                return SetCellContents(name, content);
            }
        }

        /// <summary>
        /// Property that records whether or not the spreadsheet has been modified after saving or opening.
        /// </summary>
        public override bool Changed
        {
            get
            {
                return _Changed;
            }
            protected set
            {
                _Changed = value;
            }
        }

        /// <summary>
        /// Reads in Excel XML format document. Does not handle complicated formulas, Just formulas covered by Formula class.
        /// </summary>
        /// <param name="filename"></param>
        public void ExcelReader(string filename)
        {
            //reading in the XML file
            XmlReader reader = null;
            try
            {
                reader = XmlReader.Create(filename);

                //name and contents of the cell
                string contents = null;
                int rowCount = 0;
                int colCount = 0;
                bool formulaCheck = false;
                while (reader.Read())
                {

                    if (reader.IsStartElement())
                    {

                        string formula;
                        string cell = "";
                        //checks whether the element is spreadsheet, name, or contents and handles it appropriately.
                        switch (reader.Name)
                        {
                            //if versions dont match, exception is thrown.
                            case "Row":
                                rowCount++;
                                colCount = 0;
                                if (!(null == reader.GetAttribute("ss:Index")))
                                {
                                    rowCount = int.Parse(reader.GetAttribute("ss:Index"));
                                }
                                break;

                            case "Cell":
                                colCount++;
                                formulaCheck = false;
                                cell = cellNameConverter(colCount, rowCount);
                                if (!(null == reader.GetAttribute("ss:Formula")))
                                {
                                    formula = reader.GetAttribute("ss:Formula");
                                    formulaCheck = true;
                                    formula = formulaConversion(formula, colCount, rowCount);
                                    SetContentsOfCell(cell, formula);
                                }

                                break;

                            //reads in name element
                            case "Data":
                                cell = cellNameConverter(colCount, rowCount);
                                if (!formulaCheck)
                                {
                                    reader.Read();
                                    contents = reader.Value;
                                    SetContentsOfCell(cell, contents);
                                }
                                break;
                        }
                    }
                }
            }
            catch (FileNotFoundException e)
            {
                throw new SpreadsheetReadWriteException("\"" + filename + "\"" + " does not exist.");
            }
            catch (DirectoryNotFoundException e)
            {
                throw new SpreadsheetReadWriteException("\"" + filename + "\"" + ". Directory doesnt exist.");

            }

            //catches any XmlException and throws a SpreadsheetReadWriteException, propagating the original exception's error message.
            catch (XmlException e)
            {
                throw new SpreadsheetReadWriteException(e.GetBaseException().Message);
            }
            catch (InvalidNameException e)
            {
                throw new SpreadsheetReadWriteException("An invalid cell name was used. File corruption might have occured.");
            }
            finally
            {
                if (reader != null)
                    ((IDisposable)reader).Dispose();


            }
        }

        /// <summary>
        /// Corrects Excel's weird cell names.
        /// Complicated series of steps to go from R[-5]C[0] to A1
        /// Rows and Columns are based off of current cells position. Its a relative position.
        /// EX Row in formula is +3 rows and -2 columns away from current cell.
        /// 
        /// </summary>
        /// <param name="formula"></param>
        /// <param name="col"></param>
        /// <param name="row"></param>
        /// <returns></returns>
        private string formulaConversion(string formula, int col, int row)
        {
            char[] tempCharArray = formula.ToCharArray();
            List<int> intIndexC = new List<int>();
            List<int> intIndexD = new List<int>();

            //Converting dumb excel to a standard format
            //all R's and C's without brackets are fixed.
            //it needs a string, col, row parameter
            //C
            for (int i = 0; i < tempCharArray.Count(); i++)
            {
                if ((tempCharArray[i] == 'C' && (i + 1) == tempCharArray.Count()) || (tempCharArray[i] == 'C' && tempCharArray[i + 1] != '['))
                {
                    intIndexC.Add(i + 1);
                }
            }
            int offset = 0;
            foreach (int el in intIndexC)
            {
                formula = formula.Insert(el + offset, "[" + 0 + "]");
                offset += 3;
            }
            tempCharArray = formula.ToCharArray();

            //R
            for (int i = 0; i < tempCharArray.Count(); i++)
            {
                if (tempCharArray[i] == 'R' && tempCharArray[i + 1] == 'C')
                {
                    intIndexD.Add(i + 1);
                }
            }
            offset = 0;
            foreach (int el in intIndexD)
            {
                formula = formula.Insert(el + offset, "[" + 0 + "]");
                offset += 3;
            }

            //seperates the excel variables
            MatchCollection mc = Regex.Matches(formula, @"[R](\[-?\d+\])[C](\[-?\d+\])");

            List<string> correctVariables = new List<string>();
            //grabs each excel variable from MatchCollection
            foreach (Match el in mc)
            {
                correctVariables.Add(excelVariableCorrection(el, col, row));
            }

            formula = Regex.Replace(formula, @"[R](\[-?\d+\])[C](\[-?\d+\])", "@");
            foreach (string el in correctVariables)
            {
                int indextemp = formula.IndexOf("@");
                if (indextemp > -1)
                {
                    formula = formula.Remove(indextemp, 1);
                    formula = formula.Insert(indextemp, el);
                }
            }
            return formula;
        }

        /// <summary>
        /// assumes row and columns are relative to current cell.
        /// </summary>
        /// <param name="el"></param>
        /// <returns></returns>
        private string excelVariableCorrection(Match el, int col, int row)
        {
            MatchCollection mc = Regex.Matches(el.ToString(), @"-?[\d]+");

            bool rowCalculated = false;
            StringBuilder sb = new StringBuilder();

            int newRow = 0;
            int newCol = 0;
            foreach (Match x in mc)
            {
                if (!rowCalculated)
                {
                    newRow = int.Parse(x.ToString());
                    rowCalculated = true;
                }
                else newCol = int.Parse(x.ToString());
            }

            return (char)((col + newCol) + 64) + (row + newRow).ToString();



        }

        /// <summary>
        /// Converts Row and Column numbers to Cell name.
        /// </summary>
        /// <param name="colCount"></param>
        /// <param name="rowCount"></param>
        /// <returns></returns>
        private string cellNameConverter(int colCount, int rowCount)
        {
            return ((char)(colCount + 64)).ToString() + rowCount;
        }
    }
}
