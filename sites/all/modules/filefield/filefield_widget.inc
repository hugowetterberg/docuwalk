<?php
// $Id: filefield_widget.inc,v 1.76 2009/03/31 19:02:44 quicksketch Exp $

/**
 * @file
 * This file contains CCK widget related functionality.
 *
 * Uses content.module to store the fid and field specific metadata,
 * and Drupal's files table to store the actual file data.
 */

/**
 * Implementation of CCK's hook_widget_settings($op == 'form').
 */
function filefield_widget_settings_form($widget) {
  $form = array();
  $form['file_extensions'] = array(
    '#type' => 'textfield',
    '#title' => t('Permitted upload file extensions'),
    '#default_value' => is_string($widget['file_extensions']) ? $widget['file_extensions'] : 'txt',
    '#size' => 64,
    '#description' => t('Extensions a user can upload to this field. Separate extensions with a space and do not include the leading dot. Leaving this blank will allow users to upload a file with any extension.'),
    '#weight' => 1,
  );

  $form['path_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Path settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#weight' => 6,
  );
  $form['path_settings']['file_path'] = array(
    '#type' => 'textfield',
    '#title' => t('File path'),
    '#default_value' => is_string($widget['file_path']) ? $widget['file_path'] : '',
    '#description' => t('Optional subdirectory within the "%directory" directory where files will be stored. Do not include preceding or trailing slashes.', array('%directory' => variable_get('file_directory_path', 'files') . '/')),
    '#element_validate' => array('_filefield_widget_settings_file_path_validate'),
    '#suffix' => theme('token_help', 'user'),
  );

  $form['max_filesize'] = array(
    '#type' => 'fieldset',
    '#title' => t('File size restrictions'),
    '#description' => t('Limits for the size of files that a user can upload. Note that these settings only apply to newly uploaded files, whereas existing files are not affected.'),
    '#weight' => 6,
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['max_filesize']['max_filesize_per_file'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum upload size per file'),
    '#default_value' => is_string($widget['max_filesize_per_file'])
                        ? $widget['max_filesize_per_file']
                         : '',
    '#description' => t('Specify the size limit that applies to each file separately. Enter a value like "512" (bytes), "80K" (kilobytes) or "50M" (megabytes) in order to restrict the allowed file size. If you leave this this empty the file sizes will be limited only by PHP\'s maximum post and file upload sizes (current limit <strong>%limit</strong>).', array('%limit' => format_size(file_upload_max_size()))),
    '#element_validate' => array('_filefield_widget_settings_max_filesize_per_file_validate'),
  );
  $form['max_filesize']['max_filesize_per_node'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum upload size per node'),
    '#default_value' => is_string($widget['max_filesize_per_node'])
                        ? $widget['max_filesize_per_node']
                        : '',
    '#description' => t('Specify the total size limit for all files in field on a given node. Enter a value like "512" (bytes), "80K" (kilobytes) or "50M" (megabytes) in order to restrict the total size of a node. Leave this empty if there should be no size restriction.'),
    '#element_validate' => array('_filefield_widget_settings_max_filesize_per_node_validate'),
  );

  return $form;
}

/**
 * Implementation of CCK's hook_widget_settings($op == 'save').
 */
function filefield_widget_settings_save($widget) {
  return array('file_extensions', 'file_path', 'max_filesize_per_file', 'max_filesize_per_node');
}

function _filefield_widget_settings_file_path_validate($element, &$form_state) {
  // Strip slashes from the beginning and end of $widget['file_path']
  $form_state['values']['file_path'] = trim($form_state['values']['file_path'], '\\/');

  // Do not allow the file path to be the same as the file_directory_path().
  // This causes all sorts of problems with things like file_create_url().
  if (strpos($form_state['values']['file_path'], file_directory_path()) === 0) {
    form_error($element, t('The file path (@file_path) cannot start with the system files directory (@files_directory), as this may cause conflicts when building file URLs.', array('@file_path' => $form_state['values']['file_path'], '@files_directory' => file_directory_path())));
  }
}

function _filefield_widget_settings_max_filesize_per_file_validate($element, &$form_state) {
  if (empty($form_state['values']['max_filesize_per_file'])) {
    return; // Empty means no size restrictions, so don't throw an error.
  }
  elseif (!is_numeric(parse_size($form_state['values']['max_filesize_per_file']))) {
    form_error($element, t('The "@field" option must contain a valid value. You can either leave the text field empty or enter a string like "512" (bytes), "80K" (kilobytes) or "50M" (megabytes).', array('@field' => t('Maximum upload size per file'))));
  }
}

function _filefield_widget_settings_max_filesize_per_node_validate($element, &$form_state) {
  if (empty($form_state['values']['max_filesize_per_node'])) {
    return; // Empty means no size restrictions, so don't throw an error.
  }
  elseif (!is_numeric(parse_size($form_state['values']['max_filesize_per_node']))) {
    form_error($element, t('The "@field" option must contain a valid value. You can either leave the text field empty or enter a string like "512" (bytes), "80K" (kilobytes) or "50M" (megabytes).', array('@field' => t('Maximum upload size per node'))));
  }
}

/**
 * Determine the widget's files directory
 *
 * @param $field
 *   A CCK field array.
 * @return
 *   The files directory path, with any tokens replaced.
 */
function filefield_widget_file_path($field_instance) {
  $dest = $field_instance['widget']['file_path'];
  if (module_exists('token')) {
    global $user;
    $dest = token_replace($dest, 'user', $user);
  }

  return file_directory_path() .'/'. $dest;
}

/**
 * Given a FAPI element, save any files that may have been uploaded into it.
 *
 * This function should only be called during validate, submit, or
 * value_callback functions.
 *
 * @param $element
 *   The FAPI element whose values are being saved.
 */
function filefield_save_upload($element) {
  $upload_name = $element['#field_name'] .'_'. $element['#delta'];
  $field_instance = content_fields($element['#field_name'], $element['#type_name']);

  if (empty($_FILES['files']['name'][$upload_name])) {
    return 0;
  }

  $dest = filefield_widget_file_path($field_instance);
  if (!field_file_check_directory($dest, FILE_CREATE_DIRECTORY)) {
    watchdog('filefield', 'The upload directory %directory for the file field %field (content type %type) could not be created or is not accessible. A newly uploaded file could not be saved in this directory as a consequence, and the upload was canceled.', array('%directory' => $dest, '%field' => $element['#field_name'], '%type' => $element['#type_name']));
    form_set_error($upload_name, t('The file could not be uploaded.'));
    return 0;
  }

  if (!$file = field_file_save_upload($upload_name, $element['#upload_validators'], $dest)) {
    watchdog('filefield', 'The file upload failed. %upload', array('%upload' => $upload_name));
    form_set_error($upload_name, t('The file in the @field field was unable to be uploaded.', array('@field' => $element['#title'])));
    return 0;
  }
  return $file['fid'];
}

/**
 * The #value_callback for the filefield_widget type element.
 */
function filefield_widget_value($element, $edit = FALSE) {
  if (!$edit) {
    $file = field_file_load($element['#default_value']['fid']);
    $item = $element['#default_value'];
  }
  else {
    $item = array_merge($element['#default_value'], $edit);
    $field = content_fields($element['#field_name'], $element['#type_name']);

    // Uploads take priority over value of fid text field.
    if ($fid = filefield_save_upload($element)) {
      $item['fid'] = $fid;
    }
    // Check for #filefield_value_callback values.
    // Because FAPI does not allow multiple #value_callback values like it does
    // for #element_validate and #process, this fills the missing functionality
    // to allow FileField to be extended purely through FAPI.
    elseif (isset($element['#filefield_value_callback'])) {
      foreach ($element['#filefield_value_callback'] as $callback) {
        $callback($element, $item);
      }
    }

    // Load file if the FID has changed so that it can be saved by CCK.
    $file = field_file_load($item['fid']);

    // If the file entry doesn't exist, don't save anything.
    if (empty($file)) {
      $item = array();
    }

    // Checkboxes loose their value when empty.
    // If the list field is present make sure its unchecked value is saved.
    if ($field['list_field'] && empty($edit['list'])) {
      $item['list'] = 0;
    }
  }
  // Merge file and item data so it is available to all widgets.
  $item = array_merge($item, $file);

  return $item;
}

/**
 * An element #process callback for the filefield_widget field type.
 *
 * Expands the filefield_widget type to include the upload field, upload and
 * remove buttons, and the description field.
 */
function filefield_widget_process($element, $edit, &$form_state, $form) {
  // The widget is being presented, so apply the JavaScript.
  drupal_add_js(drupal_get_path('module', 'filefield') .'/filefield.js');

  $item = $element['#value'];
  $field_name = $element['#field_name'];
  $delta = $element['#delta'];
  $element['#theme'] = 'filefield_widget_item';

  $field = content_fields($element['#field_name'], $element['#type_name']);

  // Title is not necessary for each individual field.
  if ($field['multiple'] > 0) {
    unset($element['#title']);
  }

  // Set up the buttons first since we need to check if they were clicked.
  $element['filefield_upload'] = array(
    '#type' => 'submit',
    '#value' => t('Upload'),
    '#process' => array('form_expand_ahah'),
    '#submit' => array('node_form_submit_build_node'),
    '#ahah' => array( // with JavaScript
       'path' => 'filefield/ahah/'.   $element['#type_name'] .'/'. $element['#field_name'] .'/'. $element['#delta'],
       'wrapper' => $element['#id'] .'-ahah-wrapper',
       'method' => 'replace',
       'effect' => 'fade',
    ),
    '#field_name' => $element['#field_name'],
    '#delta' => $element['#delta'],
    '#type_name' => $element['#type_name'],
    '#upload_validators' => $element['#upload_validators'],
    '#weight' => 100,
    '#post' => $element['#post'],
  );
  $element['filefield_remove'] = array(
    '#name' => $element['#field_name'] .'_'. $element['#delta'] .'_filefield_remove',
    '#type' => 'submit',
    '#value' => t('Remove'),
    '#process' => array('form_expand_ahah'),
    '#submit' => array('node_form_submit_build_node'),
    '#ahah' => array( // with JavaScript
      'path' => 'filefield/ahah/'.   $element['#type_name'] .'/'. $element['#field_name'] .'/'. $element['#delta'],
      'wrapper' => $element['#id'] .'-ahah-wrapper',
      'method' => 'replace',
      'effect' => 'fade',
    ),
    '#field_name' => $element['#field_name'],
    '#delta' => $element['#delta'],
    '#weight' => 101,
    '#post' => $element['#post'],
  );

  // Because the output of this field changes depending on the button clicked,
  // we need to ask FAPI immediately if the remove button was clicked.
  // It's not good that we call this private function, but
  // $form_state['clicked_button'] is only available after this #process
  // callback is finished.
  if (_form_button_was_clicked($element['filefield_remove'])) {
    $item = array('fid' => 0, 'list' => $field['list_default'], 'data' => array('description' => ''));
  }

  // Set access on the buttons.
  $element['filefield_upload']['#access'] = empty($item['fid']);
  $element['filefield_remove']['#access'] = !empty($item['fid']);

  // Set the FID.
  $element['fid'] = array(
    '#type' => 'hidden',
    '#value' => $item['fid'],
  );

  if ($item['fid'] != 0) {
    $element['preview'] = array(
      '#type' => 'markup',
      '#value' => theme('filefield_widget_preview', $item),
    );
  }

  // placeholder.. will be serialized into the data column. this is a place for widgets
  // to put additional data.
  $element['data'] = array(
    '#tree' => 'true',
    '#access' => !empty($item['fid']),
  );

  if ($field['description_field']) {
    $element['data']['description'] = array(
      '#type' => 'textfield',
      '#title' => t('Description'),
      '#value' => isset($item['data']['description']) ? $item['data']['description'] : '',
    );
  }

  if ($field['list_field']) {
    $element['list'] = array(
      '#type' => 'checkbox',
      '#title' => t('List'),
      '#value' => isset($item['list']) ? $item['list'] : $field['list_default'],
      '#attributes' => array('class' => 'filefield-list'),
      '#access' => !empty($item['fid']),
    );
  }
  else {
    $element['list'] = array(
      '#type' => 'hidden',
      '#value' => '1',
    );
  }

  foreach ($element['#upload_validators'] as $callback => $arguments) {
    $help_func = $callback .'_help';
    if (function_exists($help_func)) {
      $desc[] = call_user_func_array($help_func, $arguments);
    }
  }
  $element['upload'] = array(
    '#name' => 'files['. $element['#field_name'] .'_'. $element['#delta'] .']',
    '#type' => 'file',
    '#description' => implode('<br />', $desc),
    '#size' => 22,
    '#attributes' => array(
      'accept' => implode(',', array_filter(explode(' ', $field['widget']['file_extensions']))),
    ),
    '#access' => empty($item['fid']),
  );

  $element['#attributes']['id'] = $element['#id'] .'-ahah-wrapper';
  $element['#prefix'] = '<div '. drupal_attributes($element['#attributes']) .'>';
  $element['#suffix'] = '</div>';

  return $element;
}

/**
 * An #element_validate callback for the filefield_widget field.
 */
function filefield_widget_validate(&$element, &$form_state) {
  // If referencing an existing file, only allow if there are existing
  // references. This prevents unmanaged files (outside of FileField) from
  // being deleted if this node were to be deleted.
  if (!empty($element['fid']['#value'])) {
    $field = content_fields($element['#field_name'], $element['#type_name']);
    if ($file = field_file_load($element['fid']['#value'])) {
      $file = (object) $file;
      if ($file->status == FILE_STATUS_PERMANENT) {
        // TODO: We could use field_file_references() here to reference any file
        // but hook_file_delete() needs to be implemented first.
        $references = module_invoke('filefield', 'file_references', $file);
        if ($references['filefield'] == 0) {
          form_error($element, t('Referencing to the file used the %field field is not allowed.', array('%field' => $element['#title'])));
        }
      }
    }
    else {
      form_error($element, t('The file referenced by the %field field does not exist.', array('%field' => $element['#title'])));
    }
  }
}

/**
 * FormAPI theme function. Theme the output of an image field.
 */
function theme_filefield_widget($element) {
  return theme('form_element', $element, $element['#children']);
}

function theme_filefield_widget_preview($item) {
  // Remove the current description so that we get the filename as the link.
  if (isset($item['data']['description'])) {
    unset($item['data']['description']);
  }

  return '<div class="filefield-file-info">'.
           '<div class="filename">'. theme('filefield_file', $item) .'</div>'.
           '<div class="filesize">'. format_size($item['filesize']) .'</div>'.
           '<div class="filemime">'. $item['filemime'] .'</div>'.
         '</div>';
}

function theme_filefield_widget_item($element) {
  // Put the upload button directly after the upload field.
  $element['upload']['#field_suffix'] = drupal_render($element['filefield_upload']);
  $element['upload']['#theme'] = 'filefield_widget_file';

  $output = '';
  $output .= '<div class="filefield-element clear-block">';

  if ($element['fid']['#value'] != 0) {
    $output .= '<div class="widget-preview">';
    $output .= drupal_render($element['preview']);
    $output .= '</div>';
  }

  $output .= '<div class="widget-edit">';
  $output .=  drupal_render($element);
  $output .= '</div>';
  $output .= '</div>';

  return $output;
}

/**
 * Custom theme function for FileField upload elements.
 *
 * This function allows us to put the "Upload" button immediately after the
 * file upload field by respecting the #field_suffix property.
 */
function theme_filefield_widget_file($element) {
  $output = '';

  $output .= '<div class="filefield-upload clear-block">';

  if (isset($element['#field_prefix'])) {
    $output .= $element['#field_prefix'];
  }

  _form_set_class($element, array('form-file'));
  $output .= '<input type="file" name="'. $element['#name'] .'"'. ($element['#attributes'] ? ' '. drupal_attributes($element['#attributes']) : '') .' id="'. $element['#id'] .'" size="'. $element['#size'] ."\" />\n";

  if (isset($element['#field_suffix'])) {
    $output .= $element['#field_suffix'];
  }

  $output .= '</div>';

  return theme('form_element', $element, $output);
}

/**
 * Additional #validate handler for the node form.
 *
 * This function checks the #required properties on file fields and calculates
 * node upload totals for all file fields. The #required property is not
 * properly supported on file fields by Drupal core, so we do this manually.
 */
function filefield_node_form_validate($form, &$form_state) {
  $type = content_types($form['type']['#value']);
  foreach ($type['fields'] as $field_name => $field) {
    if (!(in_array($field['module'], array('imagefield', 'filefield')))) continue;
    $empty = $field['module'] .'_content_is_empty';
    $valid = FALSE;
    $total_filesize = 0;
    if (!empty($form_state['values'][$field_name])) {
      foreach ($form_state['values'][$field_name] as $delta => $item) {
        if ($empty($item, $field)) continue;
        $valid = TRUE;
        $total_filesize += (int)$item['filesize'];
      }
    }

    if (!$valid && $field['required']) {
      form_set_error($field_name, t('%title field is required.', array('%title' => $field['widget']['label'])));
    }
    $max_filesize = parse_size($field['widget']['max_filesize_per_node']);
    if ($max_filesize && $total_filesize > $max_filesize) {
      form_set_error($field_name, t('Total filesize for %title, %tsize,  exceeds field settings of %msize.',
                                    array(
                                      '%title' => $field['widget']['label'],
                                      '%tsize' => format_size($total_filesize),
                                      '%msize' => format_size($max_filesize)
                                    )
                                  ));
    }
  }
}

/**
 * Additional #submit handler for the node form.
 */
function filefield_node_form_submit($form, &$form_state) {
  // Only add additional submit handling on save.
  if ($form_state['values']['op'] == t('Save')) {
    // TODO: Immediately delete temporary files that were uploaded but never
    // saved. This isn't critical to do, since any file not saved with the node
    // will by cleaned up by system_cron() after 6 hours.
    return;
  }
}