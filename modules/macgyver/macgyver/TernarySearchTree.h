// ======================================================================
/*!
 * \brief TernarySearchTree
 *
 * This search tree works correctly for std::string keys. It has been
 * optimized a little by using the first character as a key into a trie.
 */
// ======================================================================

#pragma once

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <iostream>
#include <list>
#include <string>

namespace Fmi
{
template <typename T>
class TernarySearchTree : private boost::noncopyable
{
 public:
  typedef boost::shared_ptr<T> element_type;
  typedef std::list<element_type> result_type;

 private:
  struct Node
  {
    Node(char c);
    ~Node();

    char chr;            // the char that splits the sort
    Node* left;          // the ones earlier in alphabet
    Node* middle;        // the ones with the same next char
    Node* right;         // the ones later in alphabet
    element_type value;  // the stored value for the key which terminates here
  };

  void collect(Node* node, result_type& results) const;
  Node* root;
  size_t count;

 public:
  ~TernarySearchTree();
  TernarySearchTree();
  bool empty() const;
  size_t size() const;
  bool insert(const std::string& key, const T& data);
  bool insert(const std::string& key, element_type data);
  bool contains(const std::string& key) const;
  element_type find(const std::string& key) const;
  result_type findprefix(const std::string& key) const;
};

// ----------------------------------------------------------------------
/*!
 * \brief Node constructor
 */
// ----------------------------------------------------------------------

template <typename T>
inline TernarySearchTree<T>::Node::Node(char c)
    : chr(c), left(NULL), middle(NULL), right(NULL), value()
{
}

// ----------------------------------------------------------------------
/*!
 * \brief Node destructor
 */
// ----------------------------------------------------------------------

template <typename T>
inline TernarySearchTree<T>::Node::~Node()
{
  delete left;
  delete middle;
  delete right;
}

// ----------------------------------------------------------------------
/*!
 * \brief Destructor
 */
// ----------------------------------------------------------------------

template <typename T>
inline TernarySearchTree<T>::~TernarySearchTree()
{
  delete root;
}

// ----------------------------------------------------------------------
/*!
 * \brief Constructor
 */
// ----------------------------------------------------------------------

template <typename T>
inline TernarySearchTree<T>::TernarySearchTree()
    : root(NULL), count(0)
{
}

// ----------------------------------------------------------------------
/*!
 * \brief Test emptiness
 */
// ----------------------------------------------------------------------

template <typename T>
inline bool TernarySearchTree<T>::empty() const
{
  return (count == 0);
}

// ----------------------------------------------------------------------
/*!
 * \brief Number of values stored
 */
// ----------------------------------------------------------------------

template <typename T>
inline size_t TernarySearchTree<T>::size() const
{
  return count;
}

// ----------------------------------------------------------------------

template <typename T>
inline bool TernarySearchTree<T>::insert(const std::string& key, const T& data)
{
  return insert(key, boost::make_shared<T>(data));
}

// ----------------------------------------------------------------------

template <typename T>
inline bool TernarySearchTree<T>::insert(const std::string& key, element_type data)
{
  // Safety against empty key
  if (key.empty()) return false;

  size_t pos = 0;
  size_t n = key.size();
  Node* node = root;

  // Special case code for the first word
  if (root == NULL)
  {
    root = new Node(key[pos++]);
    node = root;

    for (; pos < n; ++pos)
    {
      node->middle = new Node(key[pos]);
      node = node->middle;
    }
    node->value = data;
    return true;
  }

  while (pos < n)
  {
    if (key[pos] < node->chr)
    {
      if (node->left == NULL)
      {
        node->left = new Node(key[pos++]);
        node = node->left;
        break;
      }
      node = node->left;
    }
    else if (key[pos] > node->chr)
    {
      if (node->right == NULL)
      {
        node->right = new Node(key[pos++]);
        node = node->right;
        break;
      }
      node = node->right;
    }
    else
    {
      if (++pos < n)
      {
        if (node->middle == NULL)
        {
          node->middle = new Node(key[pos - 1]);
          node = node->middle;
          break;
        }
        node = node->middle;
      }
    }
  }

  for (; pos < n; ++pos)
  {
    node->middle = new Node(key[pos]);
    node = node->middle;
  }

  // fail if already occupied
  if (node->value) return false;

  node->value = data;
  return true;
}

// ----------------------------------------------------------------------

template <typename T>
inline bool TernarySearchTree<T>::contains(const std::string& key) const
{
  return !find(key).empty();
}

// ----------------------------------------------------------------------

template <typename T>
inline typename TernarySearchTree<T>::element_type TernarySearchTree<T>::find(
    const std::string& key) const
{
  // Safety checks
  if (key.empty() || root == NULL) return element_type();

  Node* node = root;
  size_t n = key.size();

  for (size_t pos = 0; pos < n && node != NULL;)
  {
    if (key[pos] < node->chr)
      node = node->left;
    else if (key[pos] > node->chr)
      node = node->right;
    else
    {
      if (++pos == n) return node->value;
      node = node->middle;
    }
  }

  return element_type();
}

// ----------------------------------------------------------------------
/*!
 * \brief Collect all nodes in the subtree
 */
// ----------------------------------------------------------------------

template <typename T>
inline void TernarySearchTree<T>::collect(Node* node, result_type& results) const
{
  if (node == NULL) return;

  if (node->value) results.push_back(node->value);

  if (node->left != NULL) collect(node->left, results);
  if (node->middle != NULL) collect(node->middle, results);
  if (node->right != NULL) collect(node->right, results);
}

// ----------------------------------------------------------------------
/*!
 * \brief Find matches with the same prefix
 */
// ----------------------------------------------------------------------

template <typename T>
inline typename TernarySearchTree<T>::result_type TernarySearchTree<T>::findprefix(
    const std::string& key) const
{
  result_type results;

  // Safety checks
  if (key.empty() || root == NULL) return results;

  Node* node = root;
  size_t n = key.size();

  for (size_t pos = 0; pos < n && node != NULL;)
  {
    if (key[pos] < node->chr)
      node = node->left;
    else if (key[pos] > node->chr)
      node = node->right;
    else
    {
      if (++pos < n)
        node = node->middle;
      else
      {
        if (node->value) results.push_back(node->value);
        collect(node->middle, results);
        return results;
      }
    }
  }

  return results;
}

}  // namespace Fmi
