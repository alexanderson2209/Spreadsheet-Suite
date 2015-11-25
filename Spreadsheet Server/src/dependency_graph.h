/*
   File: dependency_graph.h
   Author: Eric Stubbs, CJ Dimaano
   Team: SegFault
   CS 3505 - Spring 2015
   Date created: April 4, 2015
   Last updated: April 23, 2015
   
   
   Resources:
   - DependencyGraph from CS 3500 Fall 2014 by CJ Dimaano.
   
*/

#ifndef DEPENDENCY_GRAPH_H
#define DEPENDENCY_GRAPH_H

#include <map>
#include <set>
#include <string>

/// <summary>
///   Represents a graph where nodes depend on other nodes.
/// </summary>
/// <remarks>
/// <para>
///   A DependencyGraph can be modeled as a set of ordered pairs of strings.
///   Two ordered pairs (s1,t1) and (s2,t2) are considered equal if and only if
///   s1 equals s2 and t1 equals t2.  (Recall that sets never contain
///   duplicates.  If an attempt is made to add an element to a set, and the
///   element is already in the set, the set remains unchanged.)
/// </para>
/// <para>
/// Given a DependencyGraph DG:
/// <OL>
///    <LI>
///      If s is a string, the set of all strings t such that (s,t) is in DG is
///      called dependents(s).
///    </LI>
///    <LI>
///      If s is a string, the set of all strings t such that (t,s) is in DG is
///      called dependees(s).
///    </LI>
/// </OL>
/// </para>
/// <para>
/// For example, suppose DG = {("a", "b"), ("a", "c"), ("b", "d"), ("d", "d")}
///     dependents("a") = {"b", "c"}
///     dependents("b") = {"d"}
///     dependents("c") = {}
///     dependents("d") = {"d"}
///     dependees("a") = {}
///     dependees("b") = {"a"}
///     dependees("c") = {"a"}
///     dependees("d") = {"b", "d"}
/// </para>
/// </remarks>
class dependency_graph
{
  
public:

  /// <summary>
  /// Copy constructor.
  /// </summary>
  dependency_graph(const dependency_graph &other);

	/// <summary>
	/// Creates an empty dependency_graph.
	/// </summary>
	dependency_graph();

	/// <summary>
	/// The number of ordered pairs in the dependency_graph.
	/// </summary>
	int size();

	/// <summary>
	/// Reports whether dependents(s) is non-empty.
	/// </summary>
	int has_dependents	(std::string s);

	/// <summary>
	/// Reports whether dependees(s) is non-empty.
	/// </summary>
	int has_dependees	(std::string s);

	/// <summary>
	/// Gets the collection of dependents(s).
	/// </summary>
	std::set<std::string> get_dependents(std::string s);

	/// <summary>
	/// Gets the collection of dependees(s).
	/// </summary>
	std::set<std::string> get_dependees	(std::string s);

	/// <summary>
	///   Adds the ordered pair (s,t), if it doesn't exist and does not cause a
  ///   circular dependency.
	/// </summary>
	/// <param name="s"></param>
	/// <param name="t"></param>
  /// <returns>
  ///   True if the dependency was added; otherwise, false.
  /// </returns>
	bool add_dependency		(std::string s, std::string t);

	/// <summary>
	/// Removes the ordered pair (s,t), if it exists.
	/// </summary>
	/// <param name="s"></param>
	/// <param name="t"></param>
	void remove_dependency	(std::string s, std::string t);

	/// <summary>
	/// Removes all existing ordered pairs of the form (s,r).  Then, for each
	/// t in newDependents, adds the ordered pair (s,t).
	/// </summary>
  /// <param name="s"></param>
  /// <param name="new_dependents"></param>
  /// <returns>
  ///   True if the dependents were replaced succesfully without circular
  ///   dependencies; otherwise, false.
  /// </returns>
	bool replace_dependents	(std::string s, std::set<std::string> new_dependents);

	/// <summary>
	/// Removes all existing ordered pairs of the form (r,s).  Then, for each 
	/// t in newDependees, adds the ordered pair (t,s).
	/// </summary>
  /// <param name="s"></param>
  /// <param name="new_dependees"></param>
  /// <returns>
  ///   True if the dependees were replaced succesfully without circular
  ///   dependencies; otherwise, false.
  /// </returns>
	bool replace_dependees	(std::string s, std::set<std::string> new_dependees);

private:

	/// <summary>
	/// Represents a collection of key-value pairs where dependees are the keys
  /// and dependents are the values.
	/// </summary>
	std::map<std::string, std::set<std::string> > dependents;

	/// <summary>
	/// Represents a collection of key-value pairs where dependents are the keys
  /// and the dependees are the values.
	/// </summary>
	std::map<std::string, std::set<std::string> > dependees;

	/// <summary>
	/// Represents the number of ordered pairs within the graph.
	/// </summary>
	int count;
  
  /// <summary>
  ///   Returns true if a circular dependency is found; otherwise, returns
  ///   false.
  /// </summary>
  /// <param name="name">
  ///   The name of the head cell to check for dependencies.
  /// </param>
  /// <returns>
  ///   True if a circular dependency is found; otherwise, false.
  /// </returns>
  /// <remarks>
  ///   This method is based solely off of the implementation of the pattern of
  ///   the C# AbstractSpreadsheet.GetCellsToRecalculate method written by Joe
  ///   Zachary for CS 3500, September 2012.
  /// </remarks>
  bool check_circular_dependents(std::string name);
  
  /// <summary>
  ///   Returns true if a circular dependency is found; otherwise, returns
  ///   false.
  /// </summary>
  /// <param name="start">
  ///   The name of the head cell.
  /// </param>
  /// <param name="name">
  ///   The name of the current cell being visited.
  /// </param>
  /// <param name="visited">
  ///   A set containing cells that have previously been visited.
  /// </param>
  /// <returns>
  ///   True if a circular dependency is found; otherwise, false.
  /// </returns>
  /// <remarks>
  /// <para>
  ///   This method is recursive.
  /// </para>
  /// <para>
  ///   This method is based solely off of the implementation of the pattern of
  ///   the C# AbstractSpreadsheet.GetCellsToRecalculate method written by Joe
  ///   Zachary for CS 3500, September 2012.
  /// </para>
  /// </remarks>
  bool visit(std::string start, std::string name,
      std::set<std::string> &visited);
  
};

#endif