<?php

/**
 * @file
 * Defines the FormValueTracker class that is used by only the FormValues class.
 */

module_load_include('inc', 'objective_forms', 'FormElement');
module_load_include('inc', 'objective_forms', 'FormElementRegistry');

/**
 * This class utilizes scope and a reference pointer to track where
 * the current value for a given FormElement. Its used by the Form values
 * class where it is used to retrieve all the values of FormElements.
 */
class FormValueTracker {

  /**
   * Submitted values from the form. A reference to $form_state['values'].
   *
   * @var array
   */
  protected $values;

  /**
   * The form element registry used to get properties for given form elements.
   *
   * @var FormElementRegistry
   */
  protected $registry;

  /**
   * A reference to a position in $values.
   *
   * @var mixed
   */
  protected $current;

  /**
   * TRUE if we are tracking a location in the values array, FALSE if not.
   *
   * @var boolean
   */
  protected $track;

  /**
   * Creates a FormValues instance.
   *
   * @param array $values
   *   Array of values
   */
  public function __construct(array &$values, FormElementRegistry $registry) {
    $this->values = &$values;
    $this->current = &$this->values;
    // Default value is FALSE.
    $this->track = FALSE;
    $this->registry = $registry;
  }

  /**
   * Gets the value for a given FormElement.
   *
   * Tracks the current position in the $values array if applicable.
   *
   * @param array $element
   *   An element in the Drupal Form.
   *
   * @return mixed
   *   Submitted value for the given FormElement if found, NULL otherwise.
   */
  public function getValue(array &$element) {
    module_load_include('inc', 'objective_forms', 'FormElementRegistry');
    $value = $this->track($element);
    // Array's are roots of #tree branchs.
    return is_array($value) ? NULL : $value;
  }

  /**
   * Get this elements index in its parent.
   *
   * @param array $element
   *   An element in the Drupal Form.
   *
   * @return mixed
   *   The index of the element if found, NULL otherwise.
   */
  protected function getIndex(array &$element) {
    if (isset($element['#array_parents'])) {
      $parents = $element['#array_parents'];
      return array_pop($parents);
    }
    elseif (isset($element['#hash'])) {
      $found = $this->registry->get($element['#hash']);
      if ($found) {
        return $found->getIndex();
      }
    }
    return NULL;
  }
  /**
   * Function track.
   *
   * Update the $current pointer so that it is pointing at the correct value.
   * Return the current position.
   *
   * @param array $element
   *   An element in the Drupal Form.
   *
   * @return mixed
   *   The value of the current position in the values tree. Typically a array
   *   or a scalar value.
   */
  protected function track(array &$element) {
    if ($this->shouldStartTracking($element)) {
      $this->track = TRUE;
    }
    if ($this->shouldStopTracking($element)) {
      $this->track = FALSE;
      $this->reset();
    }
    $index = $this->getIndex($element);
    if ($this->track && isset($this->current[$index])) {
      $this->current = &$this->current[$index];
      return $this->current;
    }
    return isset($this->current[$index]) ? $this->current[$index] : NULL;
  }

  /**
   * Checks if the tracker should begin tracking.
   *
   * @param array $element
   *   An element in the Drupal Form.
   *
   * @return bool
   *   TRUE if the tracker should start to track values, FALSE if it is already
   *   tracking or shouldn't start tracking.
   */
  protected function shouldStartTracking(array &$element) {
    return (isset($element['#tree']) && $element['#tree'] === TRUE) && !$this->track;
  }

  /**
   * Checks if the tracker should stop tracking.
   *
   * @param array $element
   *   An element in the Drupal Form.
   *
   * @return bool
   *   TRUE if the tracker should stop tracking values, FALSE if it is not
   *   tracking or should continue tracking.
   */
  protected function shouldStopTracking(array &$element) {
    return (isset($element['#tree']) && $element['#tree'] === FALSE) && $this->track;
  }

  /**
   * Reset the $current pointer to the top level of the $values array.
   */
  protected function reset() {
    $this->current = &$this->values;
  }

}
