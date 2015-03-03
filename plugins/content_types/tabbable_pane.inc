<?php

/**
 * @file
 *
 * CTools content type to render a fielded panel pane.
 */

/**
 * Small hook implementation of plugin.
 *
 * We have to use this because the form here can be loaded via form
 * caching and if this .inc file is loaded before the plugin is
 * requested, the $plugin = array() notation doesn't work.
 */

function tabbable_pane_tabbable_pane_ctools_content_types() {
  return array(
    'title' => t('Tabbable panes content'),
    'no title override' => TRUE,
    'description' => t('Create custom tabbable pane'),
    'category' => t('Tabbable panes'),
    'all contexts' => TRUE,
    'defaults' => array(
      'view_mode' => 'full',
    ),
    'edit form' => 'tabbable_pane_tabbable_pane_content_type_edit_form',
    'edit text' => t('Edit'),
  );
}


/**
 * Callback to return the custom content types with the specified $subtype_name.
 */
function tabbable_pane_tabbable_pane_content_type_content_type($subtype_name) {
  $types = _tabbable_pane_default_content_type();
  if (isset($types[$subtype_name])) {
    return $types[$subtype_name];
  }
  else {
    $entity = tabbable_pane_load_entity($subtype_name);
    if ($entity) {
      return _tabbable_pane_custom_content_type($entity);
    }
  }
}

/**
 * Callback to return all custom content types available.
 */
function tabbable_pane_tabbable_pane_content_type_content_types() {
  $types = _tabbable_pane_default_content_type();

  $ids = db_query('SELECT id FROM {tabbable_pane} WHERE reusable = 1')->fetchCol();
  if ($ids) {
    $entities = tabbable_pane_load_multiple($ids);
    foreach ($entities as $entity) {
      $subtype = _tabbable_pane_custom_content_type($entity);
      $types[$subtype['name']] = $subtype;
    }
  }

  return $types;
}

/**
 * Callback to render our content type.
 */
function tabbable_pane_tabbable_pane_content_type_render($subtype, $conf, $panel_args = array(), $context = array()) {
  $entity = tabbable_pane_load_entity($subtype);
  $view_mode = isset($conf['view_mode']) ? $conf['view_mode'] : 'full';
  if ($entity && !empty($entity->id) && tabbable_pane_access('view', $entity)) {
    $settings = field_bundle_settings('tabbable_pane', 'tabbable_pane');
    $block = new stdClass();
    $block->title = '';
    if (empty($settings['extra_fields']['display']) || !empty($settings['extra_fields']['display']['title']['default']['visible'])) {
      $block->title = !empty($entity->title) ? filter_xss_admin($entity->title) : '';
    }
    $block->content = tabbable_pane_view($entity, $view_mode);
    return $block;
  }
}

/**
 * Callback to provide the administrative title of the custom content.
 */
function tabbable_pane_tabbable_pane_content_type_admin_title($subtype, $conf) {
  $entity = tabbable_pane_load_entity($subtype);

  $output = t('Deleted/removed entity pane');
  if (isset($entity) && is_object($entity)) {
    $title = !empty($entity->admin_title) ? $entity->admin_title : (!empty($entity->title) ? $entity->title : t('No title'));

    $output = $title;
  }

  return $output;
}

/**
 * Callback to provide administrative information for a fieldable panels pane.
 */
function tabbable_pane_tabbable_pane_content_type_admin_info($subtype, $conf) {
  return tabbable_pane_tabbable_pane_content_type_render($subtype, $conf);
}

/**
 * Form used to edit our content type.
 */
function tabbable_pane_tabbable_pane_content_type_edit_form($form, &$form_state) {
  $conf = &$form_state['conf'];
  $subtype_name = $form_state['subtype_name'];
  
  if (!isset($form_state['entity'])) {
    $form_state['entity'] = tabbable_pane_load_entity($subtype_name);
  }
  $entity = $form_state['entity'];

  // It's possible that we have a reference to an entity that is no longer valid.
  // If so, bail, because otherwise field API will whitescreen.
  if (!$entity) {
    $form['error'] = array(
      '#markup' => t('The pane entity referenced does not appear to be valid. It was probably deleted and you should remove this pane.'),
    );
    return $form;
  }

  ctools_form_include_file($form_state, $form_state['plugin']['path'] . '/' . $form_state['plugin']['file']);

  // If we're adding a reusable type, the only thing we want on the form is
  // the view mode, so skip the rest.
  if ($form_state['op'] == 'add' && !empty($form_state['subtype']['entity_id'])) {
    $form_state['no update entity'] = TRUE;
    return $form;
  }
  $form = tabbable_pane_form($form, $form_state, $entity);

  $form['reusable']['warning'] = array(
    '#markup' => '<div class="description">' . t('Note: Editing any value on a reusable pane will change the value everywhere this pane is used.') . '</div>',
  );

  return $form;
}

/**
 * Validate submission of our content type edit form.
 */
function tabbable_pane_tabbable_pane_content_type_edit_form_validate($form, &$form_state) {
  if (!empty($form_state['no update entity'])) {
    return;
  }

  if ($form_state['entity']) {
    tabbable_pane_form_validate($form, $form_state);
  }
}

/**
 * Submit our content type edit form.
 */
function tabbable_pane_tabbable_pane_content_type_edit_form_submit($form, &$form_state) {
  $form_state['conf']['view_mode'] = $form_state['values']['view_mode'];

  if (!empty($form_state['no update entity'])) {
    return;
  }

  $entity = $form_state['entity'];
  if (!$entity) {
    return;
  }

  $entity = tabbable_pane_form_submit($form, $form_state);
  if (!isset($entity->is_new)) {
    // @todo -- this won't work if $form_state does not contain
    // 'pane' which could theoretically happen in a non-Panels
    // use case. Not that anybody uses this outside of Panels.
    $form_state['pane']->subtype = 'tabbable_pane:' . $entity->id;
  }
}

/**
 * Provide the default content types.
 *
 * These are all visible in the UI as the content types that allow a user
 * to add new panel pane entities that will then be stored in the database.
 */
function _tabbable_pane_default_content_type() {
  $types = array();

  $entity_info = entity_get_info('tabbable_pane');

  foreach ($entity_info['bundles'] as $bundle => $info) {
    $types[$bundle] = array(
      'name' => $bundle,
      'title' => $info['label'],
      'category' =>  t('Tabbable panes'),
      'top level' => FALSE,
      'icon' => NULL,
      'description' => t('Create a new tab UI.'),
      'all contexts' => TRUE,
      'bundle' => $bundle,
      'create content access' => 'tabbable_pane_content_type_create_access',
    );
  }

  return $types;
}

/**
 * Return an info array for a specific custom content type.
 */
function _tabbable_pane_custom_content_type($entity) {
  $info = array(
    'title' => check_plain($entity->admin_title),
    'description' => '',
    'category' => $entity->category ? check_plain($entity->category) : t('Miscellaneous'),
    'all contexts' => TRUE,
    'icon' => 'icon_block_custom.png',
    'name' => 'tabbable_pane:' . $entity->id,
    'entity_id' => 'tabbable_pane:' . $entity->id, 
  );
  return $info;
}

/**
 * Properly load the entity the subtype.
 */
function tabbable_pane_load_entity($subtype_name) {
  ctools_include('content');
  $plugin = ctools_get_content_type('tabbable_pane');

  // Next, check to see how the subtype is configured.
  $subtype_info = ctools_content_get_subtype($plugin, $subtype_name);

  // This means we're probably in the process of creating a new one.
  if (isset($subtype_info['bundle'])) {
    return tabbable_pane_create();
  }

  list($type, $id) = explode(':', $subtype_name);
  return tabbable_pane_load($id);
}

/**
 * Access callback for creating a new content type.
 */
function tabbable_pane_content_type_create_access($type, $subtype) {
  return tabbable_pane_access('create', $subtype['name']);
}

/**
 * @TODO
 */
function tabbable_pane_access($op, $name) {
  return TRUE;
}