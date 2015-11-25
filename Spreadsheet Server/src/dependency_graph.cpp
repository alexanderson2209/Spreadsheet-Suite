/*
  File: dependency_graph.cpp
  Author: Eric Stubbs, CJ Dimaano
  Team: SegFault
  CS 3505 - Spring 2015
  Date created: April 4, 2015
  Last updated: April 23, 2015
   
   
  Resources:
  - DependencyGraph from CS 3500 Fall 2014 by CJ Dimaano.
  - AbstractSpreadsheet from CS 3500 Fall 2014 by Joe Zachary.
   
*/


#include "dependency_graph.h"


/// <summary>
/// Copy constructor.
/// </summary>
dependency_graph::dependency_graph(const dependency_graph &other)
    : dependents(other.dependents), dependees(other.dependees),
      count(other.count) {
  //
  // Do nothing.
  //
}


/// <summary>
/// Creates an empty dependency_graph.
/// </summary>
dependency_graph::dependency_graph() {
	this->count = 0;
}


/// <summary>
/// The number of ordered pairs in the dependency_graph.
/// </summary>
int dependency_graph::size() {
	return this->count;
}


/// <summary>
/// Reports whether dependents(s) is non-empty.
/// </summary>
int dependency_graph::has_dependents(std::string s) {
	return this->get_dependents(s).size() > 0;
}


/// <summary>
/// Reports whether dependees(s) is non-empty.
/// </summary>
int dependency_graph::has_dependees(std::string s) {
	return this->get_dependees(s).size() > 0;
}


/// <summary>
/// Gets the collection of dependents(s).
/// </summary>
std::set<std::string> dependency_graph::get_dependents(std::string s) {
	// If s has dependents, return the set containing them.
	if (this->dependents.count(s) == 1)
		return std::set<std::string>(this->dependents[s]);
  
	// Since there are no dependents, return an empty set.
	return std::set<std::string>();
}


/// <summary>
/// Gets the collection of dependees(s).
/// </summary>
std::set<std::string> dependency_graph::get_dependees(std::string s) {
	// If s has dependees, return the set containing them.
	if (this->dependees.count(s) == 1)
		return std::set<std::string>(this->dependees[s]);

	// Since there are no dependees, return an empty set.
	return std::set<std::string>();
}


/// <summary>
///   Adds the ordered pair (s,t), if it doesn't exist and does not cause a
///   circular dependency.
/// </summary>
/// <param name="s"></param>
/// <param name="t"></param>
/// <returns>
///   True if the dependency was added; otherwise, false.
/// </returns>
bool dependency_graph::add_dependency(std::string s, std::string t) {
	// If we added a dependency, increment the count.
	if (this->dependents[s].insert(t).second
      && this->dependees[t].insert(s).second)
		this->count++;
  
  // Remove the dependency if it causes a circular exception.
  if(this->check_circular_dependents(s)) {
    this->remove_dependency(s, t);
    return false;
  }

  return true;
}


/// <summary>
/// Removes the ordered pair (s,t), if it exists.
/// </summary>
/// <param name="s"></param>
/// <param name="t"></param>
void dependency_graph::remove_dependency(std::string s, std::string t) {
	// If the dependency exists, remove it.
	if (this->dependents.count(s) == 1 && this->dependees.count(t) == 1) {
		if (this->dependents[s].erase(t) && this->dependees[t].erase(s))
			this->count--;

		// If s has no more dependents, remove it from the dictionary.
		if (this->dependents[s].size() == 0)
			this->dependents.erase(s);

		// If t has no more dependees, remove it from the dictionary.
		if (this->dependees[t].size() == 0)
			this->dependees.erase(t);
	}
}


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
bool dependency_graph::replace_dependents(std::string s,
    std::set<std::string> new_dependents) {
  // Remove (s, r) pairs.
  std::set<std::string> remove(this->get_dependents(s));
  for(std::set<std::string>::iterator it = remove.begin(); it != remove.end();
      it++)
    this->remove_dependency(s, *it);
  
  // Add (s, t) pairs.
  for(std::set<std::string>::iterator it = new_dependents.begin();
      it != new_dependents.end(); it++) {
    
    // Reset dependencies and return false if adding the current dependency
    //   results in a circular dependency.
    if(!this->add_dependency(s, *it)) {
      for(std::set<std::string>::iterator repit = remove.begin();
          repit != remove.end(); repit++)
        this->add_dependency(s, *repit);
      return false;
    }
    
  }
  
  return true;
}


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
bool dependency_graph::replace_dependees(std::string s,
    std::set<std::string> new_dependees) {
  // Remove (r, s) pairs.
  std::set<std::string> remove(this->get_dependees(s));
  for(std::set<std::string>::iterator it = remove.begin(); it != remove.end();
      it++)
    this->remove_dependency(*it, s);
  
  // Add (t, s) pairs.
  for(std::set<std::string>::iterator it = new_dependees.begin();
      it != new_dependees.end(); it++) {
        
    // Reset dependencies and return false if adding the current dependency
    //   results in a circular dependency.
    if(!this->add_dependency(*it, s)) {
      for(std::set<std::string>::iterator repit = remove.begin();
          repit != remove.end(); repit++)
        this->add_dependency(*repit, s);
      return false;
    }
    
  }
  
  return true;
}


/// <summary>
///   Returns true if a circular dependency is found; otherwise, returns false.
/// </summary>
/// <param name="name">
///   The name of the head cell to check for dependencies.
/// </param>
/// <returns>True if a circular dependency is found; otherwise, false.</returns>
/// <remarks>
///   This method is based solely off of the implementation of the pattern of
///   the C# AbstractSpreadsheet.GetCellsToRecalculate method written by Joe
///   Zachary for CS 3500, September 2012.
/// </remarks>
bool dependency_graph::check_circular_dependents(std::string name) {
  // Create a set for visited nodes.
  std::set<std::string> visited;
  
  // Walk over the graph and return true at the first instance of any circular
  //   dependencies.
  return this->visit(name, name, visited);
}


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
bool dependency_graph::visit(std::string start, std::string name,
    std::set<std::string> &visited) {
  // Add the current node to the set of visited nodes.
  visited.insert(name);
  
  // Get the direct dependents of the current node.
  std::set<std::string> dents = this->get_dependents(name);
  
  // Check each dependent if it matches the start node, and visit it if it does
  //   not match.
  for(std::set<std::string>::iterator it = dents.begin(); it != dents.end();
      it++)
  {
    // Return true if the current node matches the start node or if visiting the
    //   current results in a match for a circular dependency.
    if((start.compare(*it) == 0) || (visited.find(*it) == visited.end()
        && this->visit(start, *it, visited)))
      return true;
  }
  
  // Return false, there are no circular dependencies for the current node.
  return false;
}
