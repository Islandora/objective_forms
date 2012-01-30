<?php

/**
 * @file
 *
 * Associates submitted form values with array.
 */
module_load_include('inc', 'objective_forms', 'FormValueTracker');
module_load_include('inc', 'php_lib', 'DrupalFormHelpers');

/**
 * This class stores all submitted values. It provides a mechanism for 
 * accessing submitted values with the FormElements hashes.
 */
class FormValues {

  /**
   * A map of submitted form values where the key is the FormElement's hash.
   * 
   * @var array
   */
  protected $values;

  /**
   * A helper class that helps determine what values are associated with what elements.
   * 
   * @var FormValueTracker 
   */
  protected $tracker;

  /**
   * Checks to see if any form values were submitted to the server.
   * 
   * @param array $form_state
   *   The Drupal Form State.
   * 
   * @return boolean
   *   TRUE if $form_state['values'] exists, FALSE otherwise.
   */
  public static function Exists(array &$form_state) {
    return isset($form_state['values']);
  }

  /**
   * Create a FormValues instance.
   * 
   * @param array $form_state
   *   Drupal Form state.
   * 
   * @param array $root
   *   The elements to associate with submitted values.
   */
  public function __construct(array &$form_state, array &$root, FormElementRegistry $registry) {
    $this->values = array();
    if (self::Exists($form_state)) {
      $this->tracker = new FormValueTracker($form_state['values'], $registry);
      $this->setValues($root, $this->tracker);
    }
  }

  /**
   * Utilizes the FormValueTracker to store all the submitted values.
   * 
   * @param array $element
   *   An element in the Drupal Form.
   * 
   * @param FormValueTracker $tracker
   *   A helper class for extracting submitted values.
   */
  private function setValues(array &$element, FormValueTracker $tracker) {
    $this->setValue($element, $tracker);
    $children = element_children($element);
    foreach ($children as $key) {
      $child = &$element[$key];
      $this->setValues($child, clone $tracker);
    }
  }

  /**
   * Associate the given array &, with a submitted value.
   * 
   * @param array $element
   *   An element in the Drupal Form.
   * 
   * @param FormValueTracker $tracker 
   *   A helper class for extracting submitted values.
   */
  private function setValue(array &$element, FormValueTracker $tracker) {
    $value = $tracker->getValue($element);
    if (isset($element['#hash'])) {
      $this->values[$element['#hash']] = $value;
    }
  }

  /**
   * Is there a value associated with the given form element.
   * 
   * @param hash $hash 
   *   The unique #hash property that identifies the FormElement.
   * 
   * @return boolean
   *   TRUE if there is a value for the given FormElement.
   */
  public function hasValue($hash) {
    return isset($this->values[$hash]);
  }

  /**
   * Get the value associated with the given element.
   * 
   * @param hash $hash
   *   The unique #hash property that identifies the FormElement.
   * 
   * @return mixed
   *   The submitted value for the given element if found, NULL otherwise.
   */
  public function getValue($hash) {
    if ($this->hasValue($hash)) {
      return $this->values[$hash];
    }
    return NULL;
  }

  /**
   * Returns all values.
   * 
   * @return array 
   *   The keys of the array are the FormElement's hashs, and the values are the submitted form values.
   */
  public function getValues() {
    return $this->values;
  }

}