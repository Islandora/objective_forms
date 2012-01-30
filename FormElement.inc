<?php

/**
 * @file
 *
 * Defines a FormElement class for encapsulating Drupal form elements. Each FormElement class has a unique #hash 
 * property that can be used to identify that FormElement.
 */
module_load_include('inc', 'php_lib', 'ReflectionHelpers');
module_load_include('inc', 'php_lib', 'ReadOnlyProtectedMembers');
module_load_include('inc', 'objective_forms', 'Utils');
module_load_include('inc', 'objective_forms', 'FormElementRegistry');

/**
 * Encapsulates drupal form elements.
 */
class FormElement implements ArrayAccess {

  /**
   * Holds references to protected variables. Allows for external access and protected writes.
   *  parent -> Stores a reference to this FormElements Parent. NULL if no parent.
   *  hash -> A identifier that is unique to this object.
   * 
   * @var ReadOnlyProtected
   */
  protected $protected;

  /**
   * Child FormElements, only objects of type FormElement are stored in this array.
   * 
   * @var array
   */
  public $children;

  /**
   * Drupal form controls/properties. The values are mixed in this array.
   * 
   * All control names are prefixed with '#' for example '#title'.
   * 
   * @var array
   */
  public $controls;

  /**
   * Element Registry.
   * 
   * @var FormElementRegistry 
   */
  protected $registry;

  /**
   * Instantiate's a FormElement.
   * 
   * @param array $form
   *   Drupal form definition. 
   */
  public function __construct(FormElementRegistry $registry = NULL /* yuck fix this.. */, array $form = NULL) {
    $this->protected = new ReadOnlyProtectedMembers(array('parent' => NULL, 'hash' => NULL));
    $this->registry = $registry;
    $this->controls = array();
    $this->children = array();
    $this->initialize($form);
    $this->hash = isset($this->controls['#hash']) ? $this->controls['#hash'] : spl_object_hash($this);
    if ($this->registry) { // Deals with this case this class is used in a non user context ....
      $this->registry->registerCreated($this);
    }
  }

  /**
   * Initialize this object from a drupal form definition.
   * 
   * Takes the controls/properties/children from the drupal form definition, and creates objects repersenting 
   * elements and properties where appropriate.
   * 
   * @param array $form 
   *   Drupal form definition.
   */
  protected function initialize(array &$form = NULL) {
    if (isset($form)) {
      $this->initializeControls($form);
      $this->initializeChildren($form);
    }
  }

  /**
   * Create controls/properties from a drupal form definition.
   * 
   * @param array $form
   *   Drupal form definition.
   */
  protected function initializeControls(array &$form) {
    module_load_include('inc', 'objective_forms', 'FormProperty');
    $properties = element_properties($form);
    foreach ($properties as $key) {
      $this->controls[$key] = FormProperty::Expand($key, $form[$key]); // Objectify the property where appropriate.
    }
  }

  /**
   * Create child FormElements from a drupal form definition.
   * 
   * @param array $form 
   *   Drupal form definition.
   */
  protected function initializeChildren(array &$form) {
    $children = element_children($form);
    foreach ($children as $key) {
      $child = new FormElement($this->registry, $form[$key]);
      $this->adopt($child, $key);
    }
  }

  /**
   * Clone this FormElement. 
   * 
   * Performs a deep clone on controls and children. The cloned element won't have a parent.
   */
  public function __clone() {
    $original_hash = $this->hash;
    $this->protected = clone $this->protected;
    $this->parent = NULL; // The parent was unaware of this cloned bastard child of science...
    $this->hash = spl_object_hash($this); // Maintain uniqueness.
    // Clone Controls
    foreach ($this->controls as $key => $control) {
      if (is_array($control)) {
        $this->controls[$key] = array_copy_recursive($control);
      }
      else if (is_object($control)) {
        $this->controls[$key] = clone $control;
      }
    }
    // Clone Children.
    $children = $this->children;
    $this->children = array();
    foreach ($children as $key => $child) { // Clone Children.
      $this->adopt(clone $child, $key);
    }
    if ($this->registry) { // Deals with this case this class is used in a non user context ....
      $this->registry->registerClone($original_hash, $this);
    } 
  }

  /**
   * Search for FormElement identified by it's unique #hash property.
   * 
   * Searches this element and all of its children recursively.
   * 
   * @param hash $hash 
   *   The unique #hash property that identifies the FormElement.
   * 
   * @return FormElement
   *   The FormElement if found NULL otherwise.
   */
  public function findElement($hash) {
    if ($this->hash == $hash) {
      return $this;
    }
    foreach ($this->children as $child) {
      $element = $child->findElement($hash);
      if ($element) {
        return $element;
      }
    }
    return NULL;
  }

  /**
   * Converts the tree like structure of a FormElement and its children to a flat array.
   * 
   * @return array
   *   An array containing this FormElement and all of its children, where its keys are the FormElements unique #hash 
   *   properties. For example: array(#hash=>FormElement)
   */
  public function flatten() {
    $elements = array();
    $elements[$this->hash] = $this;
    foreach ($this->children as $child) {
      $elements = array_merge($elements, $child->flatten());
    }
    return $elements;
  }

  /**
   * Takes a callback, and calls it repeatedly passing each child FormElement to the callback
   * as the first parameter. Any other parameters passed to this function will be passed to the 
   * callback as additional parameters.
   * 
   * Unlike eachDecendant() only the immediate children of this FormElement will be used.
   * 
   * If the callback returns FALSE, then processing stops and this function exits.
   * 
   * @param callback $function
   *   The function to be called repeatedly.
   * 
   * @return boolean
   *   TRUE if the function was called for all child FormElements, FALSE otherwise.
   */
  public function eachChild($function) {
    $param_arr = array_slice(func_get_args(), 1); // Remove function name from the argument list
    foreach ($this->children as $child) {
      $args = array_merge(array($child), $param_arr);
      if (call_user_func_array($function, $args) === FALSE) {
        return FALSE;
      }
    }
    return TRUE;
  }

  /**
   * Takes a callback, and calls it repeatedly passing each control/property to the callback
   * with the name of the control/property as the first parameter and the value of the control/property
   * as the second paramter. Any other parameters passed to this function will be passed to the 
   * callback as additional parameters.
   * 
   * If the callback returns FALSE, then processing stops and this function exits.
   * 
   * @param callback $function
   *   The function to be called repeatedly.
   * 
   * @return boolean
   *   TRUE if the function was called for all controls/properties, FALSE otherwise.
   */
  public function eachControl($function) {
    $param_arr = array_slice(func_get_args(), 1); // Remove function name from the argument list
    foreach ($this->controls as $key => &$control) {
      $args = array_merge(array($key, &$control), $param_arr);
      if (call_user_func_array($function, $args) === FALSE) {
        return FALSE;
      }
    }
    return TRUE;
  }

  /**
   * Takes a callback, and calls it repeatedly passing each decendant FormElement to the callback
   * as the first parameter. Any other parameters passed to this function will be passed to the 
   * callback as additional parameters.
   * 
   * Unlike eachChild() even children of children will be used.
   * 
   * If the callback returns FALSE, then processing stops and this function exits.
   * 
   * @param callback $function
   *   The function to be called repeatedly.
   * @return boolean
   *   TRUE if the function was called for all decendants, FALSE otherwise.
   */
  public function eachDecendant($function) {
    $func_args = func_get_args();
    $param_arr = array_slice($func_args, 1); // Remove function name from the argument list
    $func_args = func_get_args();
    foreach ($this->children as $child) {
      $args = array_merge(array($child), $param_arr);
      if (call_user_func_array($function, $args) === FALSE) {
        return FALSE; // End processing
      }
      if (call_user_func_array(array($child, 'eachDecendant'), $func_args) === FALSE) { // Recurse...
        return FALSE; // End processing
      }
    }
    return TRUE; // Continue processing
  }

  /**
   * Takes a callback, and calls it repeatedly passing itself and each decendant FormElement to the callback
   * as the first parameter. Any other parameters passed to this function will be passed to the 
   * callback as additional parameters.
   * 
   * Unlike eachDecendant() this object is also used.
   * 
   * If the callback returns FALSE, then processing stops and this function exits.
   * 
   * @param callback $function
   *   The function to be called repeatedly.
   * @return boolean
   *   TRUE if the function was called for all decendants, FALSE otherwise.
   */
  public function each($function) {
    $param_arr = array_slice(func_get_args(), 1); // Remove function name from the argument list
    $args = array_merge(array($this), $param_arr);
    if (call_user_func_array($function, $args) === TRUE) {
      return call_user_func_array(array($this, 'eachDecendant'), func_get_args());
    }
    return FALSE;
  }

  /**
   * Gets a child FormElement, control/property, or ProtectedReadOnlyMember if it exists.
   * 
   * @param mixed $name 
   *   The name of the child FormElement, control/property, or ProtectedReadOnlyMember.
   * @return mixed
   *   The child FormElement, control/property, or ProtectedReadOnlyMember identified by $name if found, NULL otherwise.
   */
  public function __get($name) {
    if ($this->protected->has($name)) {
      return $this->protected->$name;
    }
    else if ($this->offsetExists("#$name")) { // Control
      return $this->offsetGet("#$name");
    }
    else if ($this->offsetExists($name)) { // Child
      return $this->offsetGet($name);
    }
    return NULL;
  }

  /**
   * Adds/Sets the value for a child FormElement, control/property, or ProtectedReadOnlyMember.
   * 
   * Can't add to the ProtectedReadOnlyMembers.
   *  
   * @param mixed $name
   *   The name of the child FormElement, control/property, or ProtectedReadOnlyMember.
   * @param mixed $value 
   *   The the child FormElement, control/property, or ProtectedReadOnlyMember to set.
   */
  public function __set($name, $value) {
    if ($this->protected->has($name)) {
      $this->protected->$name = $value;
    }
    else {
      $name = is_or_descends_from($value, 'FormElement') ? $name : "#$name";
      $this->offsetSet($name, $value);
    }
  }

  /**
   * Checks if a given child FormElement or a control/property identified by $name exists. Also checks for the existance 
   * of values stored in this objects ReadOnlyMembers.
   * 
   * @param mixed $name 
   *   The name of the child FormElement, control/property, or ProtectedReadOnlyMember.
   * 
   * @return boolean
   *   TRUE if the child FormElement, control/property, or ProtectedReadOnlyMember is set.
   */
  public function __isset($name) {
    if ($this->protected->has($name)) {
      return isset($this->protected->$name);
    }
    return $this->offsetExists($name) || $this->offsetExists("#$name");
  }

  /**
   * Unsets a given child FormElement, control/property, or ProtectedReadOnlyMember identified by $name.
   * 
   * @param mixed $name 
   *   The name of the child FormElement, control/property, or ProtectedReadOnlyMember.
   */
  public function __unset($name) {
    if ($this->protected->has($name)) {
      unset($this->protected->$name);
    }
    else if ($this->offsetExists($name)) { // Child
      $this->offsetUnset($name);
    }
    else if ($this->offsetExists("#$name")) { // Control
      $this->offsetUnset("#$name");
    }
  }

  /**
   * Checks if a given child FormElement or a control/property identified by $offsets exists.
   * 
   * @param mixed $name 
   *   The name of the child FormElement or control/property
   * 
   * @return boolean
   *   TRUE if the child FormElement or control/property exists.
   */
  public function offsetExists($offset) {
    $child_exists = isset($this->children[$offset]);
    $control_exists = isset($this->controls[$offset]);
    return $child_exists || $control_exists;
  }

  /**
   * Gets a the child FormElement or a control/property identified by $offset if it exists.
   * 
   * @param mixed $offset 
   *   The name of the child FormElement or control/property.
   * @return mixed
   *   If found, this function will return either a child FormElement or a FormControl.
   */
  public function offsetGet($offset) {
    if (array_key_exists($offset, $this->children)) {
      return $this->children[$offset];
    }
    if (array_key_exists($offset, $this->controls)) {
      return $this->controls[$offset];
    }
    return NULL;
  }

  /**
   * Adds/Sets a child FormElement or a control/property identified by $offset if it exists.
   * 
   * @param mixed $offset
   *   The name of the child FormElement or control/property.
   * @param mixed $value 
   *   Either a child FormElement or some mixed value that repersents a control/property.
   */
  public function offsetSet($offset, $value) {
    if (is_or_descends_from($value, 'FormElement')) {
      $this->adopt($value, $offset);
    }
    else if (element_property($offset)) {
      $this->controls[$offset] = $value;
    }
  }

  /**
   * Unsets a given child FormElement or control/property identified by $offset.
   * 
   * @param mixed $offset 
   *   The name of the child FormElement or control/property.
   */
  public function offsetUnset($offset) {
    if (isset($this->children[$offset])) {
      unset($this->children[$offset]);
    }
    else if (isset($this->controls[$offset])) {
      unset($this->controls[$offset]);
    }
  }

  /**
   * Adopt the child FormElement, this forcefully orphans the child from its original parent.
   * 
   * @param FormElement $child
   *   The FormElement to adopt.
   * @param mixed $key
   *   The index where the child FormElement is stored, if NULL the child is append onto the end of the array.
   */
  public function adopt(FormElement $child, $key = NULL) {
    $child->setParent($this);
    if (isset($key)) {
      if (isset($this->children[$key])) {
        $this->children[$key]->orphan();
      }
      $this->children[$key] = $child;
    }
    else {
      $this->children[] = $child;
    }
  }

  /**
   * Sets this FormElements parent to the one provided. Orphaning this FormElement from its previous parent FormElement.
   * 
   * @param FormElement $parent
   *   The new parent FormElement of this FormElement.
   */
  private function setParent(FormElement $parent) {
    $this->orphan(); // There can only be one.
    $this->parent = $parent;
  }

  /**
   * Orphans this FormElement, and removes the relationship its parent FormElement has with it.
   * 
   * @return boolean
   *   TRUE if the element was orphaned FALSE otherwise.
   */
  public function orphan() {
    if (isset($this->parent)) {
      $was_orphaned = $this->parent->orphanChild($this);
      unset($this->parent);
      return $was_orphaned;
    }
    return FALSE;
  }

  /**
   * Removes the child element defined by $remove from this FormElement. 
   * 
   * @param FormElement $remove
   *   The child FormElement to remove.
   * 
   * @return boolean
   *   TRUE if the child was removed, FALSE otherwise.
   */
  private function orphanChild(FormElement $remove) {
    foreach ($this->children as $index => $child) {
      if ($remove === $child) {
        unset($this->children[$index]);
        return TRUE;
      }
    }
    return FALSE;
  }

  /**
   * Get's the index of this FormElement in its parent's children array if it has a parent.
   * 
   * @return mixed
   *   The index of this FormElement within its parents children array. NULL if has no parent.
   */
  public function getIndex() {
    if (isset($this->parent)) {
      foreach ($this->parent->children as $index => $child) {
        if ($child === $this) {
          return $index;
        }
      }
    }
    return NULL;
  }

  /**
   * Gets the full path in the form definition to this element.
   * 
   * @return string
   *  Describes the location of this FormElement within the Form. 
   *   For example ['author']['name']
   */
  public function getLocation() {
    $index = $this->getIndex();
    return isset($index) ? "{$this->parent->getLocation()}[$index]" : '';
  }

  /**
   * Converts this FormElement into an array that can be used with the Drupal Form API, as a form definition.
   * 
   * @return array
   */
  public function toArray() {
    $output = array('#hash' => $this->hash);
    $this->addControlsToArray($output);
    $this->addChildrenToArray($output);
    return $output;
  }

  /**
   * Converts this FormElements controls/properties to an array that can be used by Drupal Form API.
   * 
   * Adds the controls to the $output parameter.
   * 
   * @param array &$output
   *   The Drupal Form API's repersentation of a form.
   */
  private function addControlsToArray(array &$output) {
    foreach ($this->controls as $name => $value) {
      $has_property_interface = has_interface($value, 'FormPropertyInterface');
      $output[$name] = $has_property_interface ? $value->toDrupalForm() : $value;
    }
  }

  /**
   * Converts this FormElements child FormElements to an array that can be used by Drupal Form API.
   * 
   * @param array &$output
   *   The Drupal Form API's repersentation of a form.
   */
  private function addChildrenToArray(array &$output) {
    foreach ($this->children as $name => $child) {
      $output[$name] = $child->toArray();
    }
  }

}