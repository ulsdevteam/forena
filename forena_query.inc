<?php
/**
 * @file forena.datablock.inc
 * Data block building and testing forms.
 */
require_once 'FrxDataSource.inc';
function forena_query_edit_block_form($form, &$form_state) {

  // Extract off the arguemnts for loading the block.
  $data = @$form_state['storage']['block'];

  $form['block'] = array(
    '#type' => 'textfield',
    '#autocomplete_path' => 'forena/data_block/autocomplete',
    '#title' => 'Data Block',
    '#required' => TRUE,
    '#description' => t('The data block to be used as the source for the data in the report.'),
  );

  $form['load'] = array(
    '#type' => 'submit',
    '#submit' => array('forena_query_load_block', 'forena_query_block_preview'),
    '#value' => 'Load',
  );

  // Only allow save as of sql data blocks.
  if ($data && $data['type']=='sql') {
    $access = forena_query_access_repository($form_state['storage']['block_name']);
    if (!@$data['locked'] && $access) {
      $form['save'] = array(
        '#type' => 'submit',
        '#value' => 'Save',
        '#submit' => array('forena_query_save_existing_block')
      );
    }
    $form['submit'] = array('#type' => 'submit', '#value' => 'Preview', '#submit' => array('forena_query_block_preview'));
    if ($access) {
      $form['save_as'] = array('#type' => 'fieldset', '#title' => 'Save As', '#collapsed' => TRUE, '#collapsible' => TRUE);
      $form['save_as']['save_as_name'] = array(
        '#type' => 'textfield',
        '#title' => t('Name'),
        '#description' => t('Indicate a proper path to save the data.  '),
      );

      $form['save_as']['save_as_submit'] = array(
        '#type' => 'submit',
        '#validate' => array('forena_query_validate_new_block'),
        '#submit' => array('forena_query_save_new_block'),
        '#value' => t('Save As')
      );
    }
  }

  $form['access'] = array(
    '#type' => 'item',
    '#title' => 'Access',
    '#markup' => $data['access'],
  );
  if ($data && $data['type']=='sql') {
    $form['file'] = array(
      '#type' => 'textarea',
      '#title' => t('Source'),
      '#disabled' => @$data['locked'],
      '#default_value' => @$data['file'],
    );
  }

  if (@$data['tokens']) {

    $form['parameters'] = array(
      '#title' => 'Parameters',
      '#type' => 'fieldset',
      '#tree' => TRUE
    );

    foreach ($data['tokens'] as $name) {
      $ctl = array(
        '#type' => 'textfield',
        '#title' => check_plain($name),
      );
      $form['parameters'][$name] = $ctl;
    }
  }

  $form['preview'] = array('#markup' => @$form_state['storage']['preview']);

  return $form;
}

function forena_query_block_preview($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $block = $form_state['values']['block'];
  $block_def = isset($form_state['storage']['block']) ? $form_state['storage']['block'] : array();
  $parms = isset($form_state['values']['parameters']) ? $form_state['values']['parameters'] : array();
  $block_name = $form_state['storage']['block_name'];
  FrxReportGenerator::instance()->alter_parameters('', $parms);
  if ($block_def['type'] == 'sql') {
    $sql = $form_state['values']['file'];
    list($provider, $block_name) = @explode('/', $block, 2);
    $new_block = Frx::RepoMan()->sqlBlock($provider, $sql);
    if ($new_block) $block_def = array_merge($block_def, $new_block);
    $form_state['storage']['block'] = $block_def;
    $data = Frx::RepoMan()->sqlData($provider, $sql, $parms);
  }
  else {
    $data = Frx::RepoMan()->data($block_name);
  }
  $t = Frx::Template('FrxTable');
  $config = array('class' => 'dataTable', 'id' => 'preview-data');
  if ($data && $t) {
    $t->generate($data, $config);
    $fields = $t->columns;
    $form_state['storage']['fields'] = array_combine($fields, $fields);

    $report = $t->asXML();
    $rpt = new FrxReport($report, $data);
    $form_state['storage']['preview'] = $rpt->render('web');
  }
  else {
    $form_state['storage']['preview'] = '';
  }
  if (is_object($data)) $data = $data->asXML();

}

function forena_query_load_block($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $form_state['storage']['block'] = Frx::RepoMan()->loadBlock($form_state['values']['block']);
  $form_state['storage']['block_name'] = $form_state['values']['block'];
  $form_state['input']['file'] = $form_state['values']['file'] = @$form_state['storage']['block']['file'];
}

function forena_query_validate_new_block($form, &$form_state) {

  // Check to make sure a filename is specified.

  $block_name = $form_state['values']['save_as_name'];
  if (!$block_name) {
    form_set_error('save_as_name',  t("Block_name is required!"));
    return;
  }

  list($src_repos, $name) = @explode('/', $form_state['values']['block']);
  list($dest_repos, $name) = @explode('/', $block_name);
  if ($src_repos != $dest_repos) {
    form_set_error('save_as_name', t('Block must be in the same data source/repository'));
    return;
  }

  $block = Frx::RepoMan()->loadBlock($block_name);
  if ($block) {
    form_set_error('save_as_name', t('Block already exists'));
    return;
  }
}

/**
 * Save one block as another.
 * @param unknown_type $form
 * @param unknown_type $form_state
 */
function forena_query_save_new_block($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $block_name = $form_state['values']['save_as_name'];
  $data = $form_state['storage']['block'];
  $form_state['storage']['block']['locked']=FALSE;
  Frx::RepoMan()->saveBlock($block_name, $data);
  $form_state['input']['block'] = $block_name;
  $form_state['storage']['block_name'] = $block_name;
  drupal_set_message('Block Saved');
}

function forena_query_save_existing_block($form, &$form_state) {
  $block_name = $form_state['storage']['block_name'];
  $data = $form_state['storage']['block'];
  $data['file'] = $form_state['values']['file'];
  Frx::RepoMan()->saveBlock($block_name, $data);
  drupal_set_message('Block Saved');
  $form_state['input']['block'] = $block_name;
  $form_state['rebuild'] = TRUE;
}

/**
 * Recursively clean all submit values from form elements
 * assuming tree = true structure.
 * @param $values
 */
function _forena_query_clean_values(&$values, &$elements) {
  foreach($elements as $key=>$value) if (strpos($key, '#')!==0) {
    if (@$value['#type']=='submit') {
      unset($values[$key]);
    }
    else {
      _forena_query_clean_values($values[$key], $elements[$key]);
    }
  }
}


/**
 * Implements query builder form.
 * @param $formid
 * @param $form_state
 */
function forena_query_builder_form($formid, &$form_state) {
  // Define an empty block
  $default_storage = array(
    'block' => array(),
    'block_name' => '',
    'builder' => array(
      'where' => array(
        'op' => 'AND',
        'filter' => array(
          array('field' => '', 'op' => '', 'value' => ''),
        ),
      ),
      'group' => array(),
      'columns' => array(),
      ),
    'fields' => array(),
    'step' => 'block',
    'preview' => '',
  );

  // Work with the existing block or load default
  if (!isset($form_state['storage'])) $form_state['storage'] = $default_storage;
  $data = &$form_state['storage'];

  if ($data['step'] == 'block') {
    $form['block'] = array(
      '#type' => 'textfield',
      '#autocomplete_path' => 'forena/data_block/autocomplete',
      '#title' => 'Data Block',
      '#required' => TRUE,
      '#description' => t('The source data block for the report'),
    );

    $form['load'] = array(
      '#type' => 'submit',
      '#submit' => array('forena_query_load_block', 'forena_query_block_preview', 'forena_query_builder_next'),
      '#value' => 'Load',
    );

    $form['next'] = array(
     '#type' => 'value',
     '#value' => 'where',
    );
  }

  if ($data['step'] == 'where') {
    $form['where'] = array('#tree' => TRUE);
    $form['save'] = array(
    	'#type' => 'submit',
      '#value' => t('Save'),
      '#submit' => array('forena_query_builder_save'),
    );
    $form['preview'] = array(
      '#type' => 'markup',
      '#markup' => $data['preview'],
    );
    $form['where'] = array(
      '#type' => 'fieldset',
      '#title' => 'Filter Criteria',
      '#tree' => TRUE,
    );
    _forena_query_filter_ctl($data['builder']['where'], $form['where'], $form_state['storage']['fields']);
  }


  return $form;

}

/**
 * Recursively build query form.
 * @param $data array of data where clause filters
 * @param $ctl form control to modify
 * @param unknown_type $fields
 */
function _forena_query_filter_ctl(&$data, &$ctl, $fields) {
  static $i = 0;
  $i++;
  $operators = array('=' => '=', '<>' => '<>', 'IS NOT NULL' => 'is not null', 'is NULL' => 'is null');
  $ctl['op'] = array(
    '#type' => 'select',
    '#options' => array('AND' => 'all of', 'OR' => 'any of' ),
    '#default_value' => $data['op'],
  );
  $ctl['filter'] = array('#type' => 'fieldset');
  if ($i>1) $ctl['filter']['ungroup'] = array(
    '#type' => 'submit',
    '#value' => t('Remove'),
    '#name' => $i,
    '#submit' => array('forena_query_remove_group'),
    '#weight' => 1);

  $i++;
  foreach ($data['filter'] as $k=>$crit) {
    $i++;
    if (isset($crit['filter'])) {
      $cnd = array();
      _forena_query_filter_ctl($data['filter'][$k], $cnd, $fields);
    }
    else {
      $cnd = array('#prefix' => '<div class="forena-filter">', '#suffix' => '</div>');
      $cnd['field'] = array('#type' => 'select', '#options' => $fields, '#default_value' => $crit['field']);
      $cnd['op'] = array('#type' => 'select', '#options' => $operators, '#default_value' => $crit['op']);
      $cnd['value'] = array('#type' => 'textfield', '#default_value' => $crit['value'] );
      $cnd['group'] = array('#type' => 'submit', '#value' => '( )', '#submit' => array('forena_query_group_filter'), '#name' => $i);
      $i++;
      if ($k>0) $cnd['remove_filter'] = array('#type' => 'submit', '#value' => '-', '#submit' => array('forena_query_remove_filter'), '#name' => $i);
      $i++;
    }
    $ctl['filter'][]  = $cnd;
  }
  $i++;
  $ctl['filter']['add_filter'] = array('#type' => 'submit', '#value' => '+', '#submit' => array('forena_query_add_filter'), '#name' => $i);
}

function forena_query_builder_save($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $values = $form_state['values'];
  _forena_query_clean_values($values['where'], $form['where']);
  $form_state['storage']['preview'] = '<pre>' . print_r($values['where'],1) . '</pre>';

}

/**
 * Assuming a tree=true structure, drill down into parent to
 * add a data structure to the appropriate point in the tree.
 * @param $values data from form elements
 * @param $parents array of parents that indicates how deep to set the tree.
 * @param $to_add element to add
 */
function _forena_query_add_parent_data(&$values, &$parents, $to_add) {
  if ($parents) {
    $key = array_shift($parents);
    _forena_query_add_parent_data($values[$key], $parents, $to_add);
  }
  else {
    $values[]=$to_add;
  }
}

/**
 * Add another field to the filter.
 * @param unknown_type $form
 * @param unknown_type $form_state
 */
function forena_query_add_filter($form, &$form_state) {
  $where = $form_state['values']['where'];
  _forena_query_clean_values($where, $form['where']);
  // Determine containing field of pressed button.
  $parents = $form_state['triggering_element']['#parents'];
  array_shift($parents);
  array_pop($parents);
  $field = array_fill_keys(array('field', 'op', 'value'),'');
  _forena_query_add_parent_data($where, $parents, $field);
  $form_state['storage']['builder']['where'] = $where;
  $form_state['rebuild'] = TRUE;

}

/**
 * Assuming a tree=true structure, drill down into parent to
 * add a data structure to the appropriate point in the tree.
 * @param $values data from form elements
 * @param $parents array of parents that indicates how deep to set the tree.
 * @param $to_add element to add
 */
function _forena_query_remove_parent_data(&$values, &$parents) {
   if ($parents) {
    $key = array_shift($parents);
    if (!$parents) {
      unset($values[$key]);
    }
    else {
      _forena_query_remove_parent_data($values[$key], $parents);
    }
  }
}

/**
 *
 * Remove a filter grouping.
 * @param unknown_type $form
 * @param unknown_type $form_state
 */
function forena_query_remove_filter($form, &$form_state) {
  $where = $form_state['values']['where'];
  _forena_query_clean_values($where, $form['where']);
  // Determine containing field of pressed button.
  $parents = $form_state['triggering_element']['#parents'];
  array_shift($parents);
  array_pop($parents);
  _forena_query_remove_parent_data($where, $parents);
  $form_state['storage']['builder']['where'] = $where;
  $form_state['rebuild'] = TRUE;
}

/**
 *
 * Remove a grouping.
 * @param unknown_type $form
 * @param unknown_type $form_state
 */
function forena_query_remove_group($form, &$form_state) {
  $where = $form_state['values']['where'];
  _forena_query_clean_values($where, $form['where']);
  // Determine containing field of pressed button.
  $parents = $form_state['triggering_element']['#parents'];
  array_shift($parents);
  array_pop($parents);
  array_pop($parents);
  _forena_query_remove_parent_data($where, $parents);
  if (!$where['filter']) {
    $where['filter'][] =  array_fill_keys(array('field', 'op', 'value'),'');
  }
  $form_state['storage']['builder']['where'] = $where;
  $form_state['rebuild'] = TRUE;
}

function _forena_query_group_parent_data(&$values, &$parents) {
  if ($parents) {
    $key = array_shift($parents);
    _forena_query_group_parent_data($values[$key], $parents);
  }
  else {
    $filter = $values;
    $values = array(
      'op' => 'AND',
      'filter' => array(),
    );
    $values['filter'][] = $filter;
  }
}


/**
 * Group a filter
 * @param unknown_type $form
 * @param unknown_type $form_state
 */
function forena_query_group_filter($form, &$form_state) {
   $where = $form_state['values']['where'];
  _forena_query_clean_values($where, $form['where']);
  // Determine containing field of pressed button.
  $parents = $form_state['triggering_element']['#parents'];
  array_shift($parents);
  array_pop($parents);
  _forena_query_group_parent_data($where, $parents);
  $form_state['storage']['builder']['where'] = $where;
  $form_state['rebuild'] = TRUE;
}

/**
 * Submit handler to validate the next step in the query builder
 */
function forena_query_builder_next($form, &$form_state) {
  $next = $form_state['values']['next'];
  $data = $form_state['storage'];
  $step = 'block';
  switch ($next) {
    case 'where':
      if ($data['preview'] && $data['block_name']) {
        $step = $next;
      }
      break;
    default:
      $step = 'block';
  }
  $form_state['storage']['step'] = $step;
}




