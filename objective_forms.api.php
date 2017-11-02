<?php

/**
 * @file
 * This file documents hooks provided by Objective Forms.
 */

/**
 * Define a custom Drupal form property.
 *
 * @return array
 *   Returns an associative array mapping property names to associative
 *   arrays with all the data required to build a FormProperty class:
 *   - "type": The type (file extension) of the file to include, e.g. "inc".
 *   - "module": The machine name of the module where the code is located.
 *   - "name": The name of the file, including its path within the module.
 *   - "class": The name of the class defining the property.
 */
function hook_objectify_properties() {
  return array(
    '#actions' => array(
      'type' => 'inc',
      'module' => 'module_name',
      'file' => 'includes/FileName',
      'class' => 'ClassName',
    ),
  );
}
