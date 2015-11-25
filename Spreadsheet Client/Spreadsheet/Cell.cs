using SpreadsheetUtilities;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SS
{
    /// <summary>
    /// Cell class. Holds the cell's name, its contents, and its value.
    /// 
    /// Cell class is used for a spreadsheet class. 
    /// 
    /// Its contents hold an object.
    /// Its value holds an object.
    /// 
    /// This is to allow multiple types to be used for contents and value.
    /// </summary>
    class Cell
    {

        /// <summary>
        /// Name of the cell
        /// </summary>
        public string name;


        /// <summary>
        ///Contents is an object. The contents of the cell can be anything that extends object.
        ///It allows for flexible code, but it is prone to abundant casting.
        /// </summary>
        public Object contents;

        /// <summary>
        ///value is an object. The value of the cell can be anything that extends object.
        ///It allows for flexible code, but it is prone to abundant casting.
        /// </summary>
        public Object value;

        /// <summary>
        /// Creates new cell passing name, contents, and value arguments. 
        /// </summary>
        /// <param name="name"></param>
        /// <param name="contents"></param>
        /// <param name="value"></param>
        public Cell(string name, Object contents, Object value)
        {
            this.name = name;
            this.contents = contents;
            this.value = value;
        }



    }
}
