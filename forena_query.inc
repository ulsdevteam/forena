<?php
/**
 * @file forena.datablock.inc
 * Data block building and testing forms.
 */
require_once 'FrxDataSource.inc';


function forena_query_display_blocks() {

  $header = array(t('Block'), t('Access'), t('Keywords'), t('Actions'));

  $blocks = Frx::DataFile()->userBlocks();
  $links[] = array('href' => 'admin/structure/forena/data/add', 'title' => 'Create New SQL Query)' );
  $content['add']['#markup'] = theme('links',
      array(
          'links' => $links,
          'attributes' => array('class' => 'action-links'),
      )
  );
  $data=array();
  foreach ($blocks as $block_name => $block) {
    $blck = str_replace('/', '.', $block_name);
    $actions = '';
    if ($block->cache['type'] == 'sql') {
      if ($block->include) {
        $actions = l(t('Override'), 'admin/structure/forena/data/' . $blck . '/edit');
      }
      else {
        $actions = l(t('Edit'), 'admin/structure/forena/data/' . $blck . '/edit');
      }
      $clone = l(t('Clone'), 'admin/structure/forena/data/clone/' . $blck);
      // Determine the nature of the report delete link.
      if ($block->override && $block->include) {
        $actions .= ' ' .l(t('Revert'), 'admin/structure/forena/data/' . $blck . '/delete', array('query' => array('destination' => 'admin/structure/forena/data')));
      }
      else if (!$block->include) {
        $actions .= ' ' . l(t('Delete'), 'admin/structure/forena/data/' . $blck . '/delete', array('query' => array('destination' => 'admin/structure/forena/data')));
      }
    }
    // Build the table.
    $data[] = array(
    	$block_name,
      @$block->cache['access'],
      @$block->cache['keywords'],
      $actions,
    );
  }
  // Theme the table
  $content['data_blocks'] = array('#markup' => theme('forena_data_table', array('header' => $header, 'rows' => $data)));
  return $content;
}

/**
 * Create new sql data form hook
 * @param unknown $form
 * @param unknown $form_state
 * @return unknown
 */
function forena_query_create_block_form($form, &$form_state, $provider='') {
  $providers = Frx::RepoMan()->repositories;
  $form['provider'] = array(
    '#required' => TRUE,
    '#title' => t('Data Source'),
    '#type' => 'select',
    '#options' => Frx::RepoMan()->listRepos(),
    '#default_value' => $provider,

  );

  $form['block_name'] = array(
  	'#type' => 'textfield',
    '#required' => TRUE,
    '#title' => t('Block to create'),
  );

  $form['add'] = array(
  	'#type' => 'submit',
    '#value' => t('Create'),
  );
  return $form;
}

function forena_query_create_block_form_validate($form, &$form_state) {
  extract ($form_state['values']);
  $pattern = "@^[A-Za-z0-9\/\_]$@";
  if (preg_match($pattern, $block_name)) {
    form_set_error('block_name', t('Invalid characters in block name') );
  }
  else {
    $path = $provider . '/' . $block_name;
    $block = Frx::RepoMan()->loadBlock($path);
    if ($block) form_set_error('block_name', t('Data Block already exists'));
  }
}

function forena_query_create_block_form_submit($form, &$form_state) {
  extract($form_state['values']);
  $path = $provider . '/' . $block_name;
  Frx::BlockEditor($path)->update(
  	array('type' => 'sql',
  	      'access' => '',
  	      'file' => '',
  	      'source' => '',
  	)
  );
  $form_state['redirect'] = 'admin/structure/forena/data/' . str_replace('/', '.', $path) . "/edit";
}

/**
 * Basic SQL Editor form hook implementation
 * @param unknown $form
 * @param unknown $form_state
 * @param string $block_name
 * @return multitype:NULL
 */
function forena_query_sql_editor($form, &$form_state, $block_name='') {
  if (!isset($form_state['storage'])) {
    $form_state['storage']['block_name'] = $block_name;
    $form_state['storage']['parameters'] = array();
  }
  $block_name = $form_state['storage']['block_name'];
  $block = Frx::BlockEditor($block_name)->block;

  $form['access'] = array(
      '#title' => t('Data Security'),
      '#description' => t('Value depends on the security for the data block,
          but for drupal data, this is a right as defined by the module provided.  '),
      '#type' => 'textfield',
      '#disabled' => !user_access('forena data security'),
      '#default_value' => $block['access'],
  );

  $form['file'] = array(
      '#type' => 'textarea',
      '#title' => t('Source'),
      '#default_value' => @$block['file'],
  );

  $form['save'] = array(
      '#type' => 'submit',
      '#value' => 'Save',
      '#validate' => array('forena_query_validate_block'),
      '#submit' => array('forena_query_editor_update', 'forena_query_save_block')
  );
  $form['cancel'] = array('#type' => 'submit', '#value' => t('Cancel'), '#submit' => array('forena_query_close'), '#limit_validation_errors' => array());

  if (@$block['tokens']) {
    $form['parameters'] = array(
        '#title' => 'Parameters',
        '#type' => 'fieldset',
        '#tree' => TRUE
    );

    foreach ($block['tokens'] as $name) {
      $ctl = array(
          '#type' => 'textfield',
          '#title' => check_plain($name),
      );
      $form['parameters'][$name] = $ctl;
    }
  }

  $form['submit'] = array('#type' => 'submit', '#value' => t('Test Changes'), '#submit' => array('forena_query_editor_update', 'forena_query_preview'));
  $form['preview'] = array('#markup' => @$form_state['storage']['preview']);
  return $form;
}


function forena_query_builder_form($formid, &$form_state) {
  // Define an empty block
  $access = TRUE;
  $default_storage = array(
    'block' => array(),
    'block_name' => '',
    'parameters' => array(),
    'provider' => '',
    'builder' => array(
      'columns' => array(),
      'block' => '',
      'where' => array(
        'op' => 'AND',
        'filter' => array(
  array('field' => '', 'op' => '', 'value' => ''),
  ),
  ),
      'group' => array(),
      'order' => array(),
  ),
    'fields' => array(),
    'step' => 'block',
    'preview' => '',
    'new' => TRUE,
  );

  // Work with the existing block or load default
  if (!isset($form_state['storage']))  {
    $form_state['storage'] = $default_storage;

  }
  $data = &$form_state['storage'];
  $block = $data['block'];
  $step = $form_state['storage']['step'];

  /**
   * Block select portion of ofrm
   */
  // First stage is to either load an existing block or create a new one
  if ($step == 'block') {

    $form['block'] = array(
      '#type' => 'textfield',
      '#autocomplete_path' => 'forena/data_block/autocomplete',
      '#title' => 'Data Block',
      '#required' => TRUE,
      '#description' => t('Select an exising block or create a new one derived from the selected block.'),
    );

    $form['load_include'] = array(
      '#type' => 'submit',
      '#validate' => array('forena_query_validate_block'),
      '#submit' => array('forena_query_load_include', 'forena_query_preview'),
      '#value' => t('New'),
    );

    $form['load'] = array(
      '#type' => 'submit',
      '#validate' => array('forena_query_validate_block'),
      '#submit' => array('forena_query_load_block', 'forena_query_preview'),
      '#value' => t('Load'),
    );

  }
  // If we're not on the first form than we need to verify the acces for the block
  else {
    $access = forena_query_access_repository($form_state['storage']['block_name']);
    /**
     * Section of form for finally saving the report.
     */
    if ($access && $form_state['storage']['new']) {
      $form['save_as_name'] = array(
        '#type' => 'textfield',
        '#title' => t('Name') . ' ' . $form_state['storage']['provider'] . '/',
        '#description' => t('Name of block to save'),
      );
    }

    if (@$form_state['storage']['delete']) {
      $form['question'] = array(
        '#type' => 'markup',
        '#markup' => '<p>Are you sure you want to delete ' . $data['block_name'] . '?</p>',
      );

      $form['confirm'] = array(
        '#type' => 'submit',
        '#submit' => array('forena_query_delete_block'),
        '#value' => t('Yes'),
      );
      $form['cancel'] = array(
        '#type' => 'submit',
        '#submit' => array('forena_query_delete_cancel'),
        '#value' => t('No'),
      );
      return $form;
    }

    if ($step == 'builder' || $step == 'sql') {
      $form['access'] = array(
        '#title' => t('Data Security'),
        '#description' => t('Value depends on the security for the data block,
          but for drupal data, this is a right as defined by the module provided.  '),
        '#type' => 'textfield',
        '#required' => TRUE,
        '#disabled' => !user_access('forena data security') || @$block['locked'],
        '#default_value' => $block['access'],
      );
    }

    /**
     * Edit query builder clause portion of form.
     */
    if ($step == 'builder') {
      $form['where'] = array('#tree' => TRUE);

      $form['where'] = array(
        '#type' => 'fieldset',
        '#title' => 'Filter Criteria',
        '#tree' => TRUE,
      );
      _forena_query_filter_ctl($data['builder']['where'], $form['where'], $form_state['storage']['fields']);
      $form['sql'] = array(
        '#type' => 'submit',
        '#value' => t('SQL'),
        '#submit' => array('forena_query_update_builder', 'forena_query_sql_view'),
      );
    }

    /**
     * Edit SQL directly.
     */
    if ($step == 'sql') {

      if ($block && $block['type']=='sql') {
        $form['file'] = array(
          '#type' => 'textarea',
          '#title' => t('Source'),
          '#default_value' => @$block['file'],
        );

        if (@$block['builder']) {
          $form['builder_view'] = array(
            '#type' => 'submit',
            '#value' => t('Query Builder'),
            '#submit' => array('forena_query_builder_view'),
          );
        }


      }


    }
    if (@$block['tokens']) {
      $form['parameters'] = array(
        '#title' => 'Parameters',
        '#type' => 'fieldset',
        '#tree' => TRUE
      );

      foreach ($block['tokens'] as $name) {
        $ctl = array(
          '#type' => 'textfield',
          '#title' => check_plain($name),
        );
        $form['parameters'][$name] = $ctl;
      }
    }

    if (!@$form_state['storage']['new'] && $step != $block) {
      $form['block'] = array(
        '#type' => 'value',
        '#value' => $data['block_name'],
      );

      $form['load'] = array(
        '#type' => 'submit',
        '#submit' => array('forena_query_load_block', 'forena_query_preview'),
        '#value' => t('Reload'),
      );

    }

    if (!@$block['locked'] && $access) {
      $form['save'] = array(
        '#type' => 'submit',
        '#value' => 'Save',
        '#validate' => array('forena_query_validate_new_block'),
        '#submit' => array('forena_query_editor_update', 'forena_query_save_block')
      );

      if (!$data['new']) $form['delete'] = array('#type' => 'submit', '#value' => t('Delete'), '#submit' => array( 'forena_query_try_delete'));
    }
    $form['submit'] = array('#type' => 'submit', '#value' => t('Preview'), '#submit' => array('forena_query_editor_update', 'forena_query_preview'));
    $form['cancel'] = array('#type' => 'submit', '#value' => t('Cancel'), '#submit' => array('forena_query_close'), '#limit_validation_errors' => array());
    $form['preview'] = array('#markup' => @$form_state['storage']['preview']);
  }

  return $form;

}

function forena_query_load_block($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $form_state['storage']['new'] = FALSE;
  @list($provider, $block) = @explode('/', $form_state['values']['block'], 2);
  $block_def = @Frx::RepoMan()->loadBlock($form_state['values']['block']);
  $form_state['storage']['block'] = $block_def;
  $form_state['storage']['provider'] = $provider;
  $form_state['storage']['block_name'] = $form_state['values']['block'];
  if (@$block_def['builder']) {
    $form_state['storage']['builder'] = $block_def['builder'];
    $form_state['storage']['step'] = 'builder';
  }
  else {
    $form_state['storage']['step'] =  'sql';
  }

}

function forena_query_validate_block($form, &$form_state) {
   if (isset($form_state['values']['access'])) {
     $o = Frx::BlockEditor();
     $o->load($form_state['storage']['block_name']);
     $r = $o->repos();
     $access = $r->access($form_state['values']['access']);
     if (!$access) {
       form_set_error('access', t('You must be granted access to use this right.'));
     }
   }
   if (!Frx::DataFile()->isWritable(Frx::BlockEditor()->block_name . '.sql')) {
     form_set_error('block', t('Insufficient Permission to save file'));
   }


}

/**
 * Load include block as source of builder.
 * @param $form
 * @param $form_state
 */
function forena_query_load_include($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  @list($provider, $block) = @explode('/', $form_state['values']['block'], 2);
  $block_def = @Frx::RepoMan()->loadBlock($form_state['values']['block']);
  if (!$block_def) {
    $block_def = array(
      'access' => '',
      'type' => 'sql',
      'file' => '',
      'source' => '',
    );
    $form_state['storage']['step'] = 'sql';
  }
  else {
    $form_state['storage']['step'] = 'builder';
  }
  $form_state['storage']['block'] = $block_def;
  $form_state['storage']['block']['locked'] = FALSE;
  $form_state['storage']['provider'] = $provider;
  $form_state['storage']['builder']['block_name'] = $block;
  $form_state['storage']['block_name'] = $form_state['values']['block'];
  $form_state['storage']['new'] = TRUE;
  $form_state['input']['file'] = $form_state['values']['file'] = @$form_state['storage']['block']['file'];
}

/**
 * Submit handler to update the form fields.
 */
function forena_query_update_sql($form, &$form_state) {
  $file = $form_state['values']['file'];
  $provider = $form_state['storage']['provider'];
  $block_def = $form_state['storage']['block'];
  $block_def['access'] = $form_state['values']['access'];
  if ($block_def['file']!=$file) {
    $block_def['builder'] = '';
    $new_block = Frx::RepoMan()->sqlBlock($provider, $file);
    if ($new_block) $block_def = array_merge($block_def, $new_block);
  }
  $form_state['storage']['block'] = $block_def;
}

function forena_query_preview($form, &$form_state) {
  $form_state['rebuild'] = TRUE;

  $parms = $form_state['storage']['parameters'];
  if (isset($form_state['values']['parameters'])) {
    $parms = $form_state['storage']['parameters'] = $form_state['values']['parameters'];
  }
  $form_state['storage']['preview'] = Frx::BlockEditor()->preview($parms);
}

function forena_query_delete_form($form, &$form_state, $block_name) {
  $block_name = str_replace('.', '/', $block_name);
  $o =  Frx::BlockEditor($block_name);
  $block = $o->block;
  $file = @$block['file'];
  if ($o->modified) {
    $o->cancel();
    drupal_not_found($block_name);
    exit;
  }

  $form_state['storage']['block_name'] = $block_name;

  $form['block_title'] = array('#markup' => "<h2>$block_name</h2>");

  $form['file'] = array('#markup' => "<pre>$file</pre>");

  $form['confirm'] = array(
    '#type' => 'checkbox',
    '#title' => t('Are you sure you want to delete this block?'),
    '#required' => TRUE,
  );

  $form['block_name'] = array(
  	'#type' => 'value',
    '#value' => $o->block_name,
  );

  $form['delete'] = array(
  	'#type' => 'submit',
    '#value' => t('Delete'),
    '#submit' => array('forena_query_delete_block', 'forena_query_close'),
  );
  $form['cancel'] = array('#type' => 'submit', '#value' => t('Cancel'), '#submit' => array('forena_query_close'), '#limit_validation_errors' => array());
  return $form;
}


function forena_query_save_block($form, &$form_state) {
  $form_state['rebuild'] = FALSE;
  Frx::BlockEditor()->update($form_state['values'])->save();
}

function forena_query_delete_block($form, &$form_state) {
  $block_name = $form_state['values']['block_name'];
  Frx::RepoMan()->deleteBlock($block_name);
  $form_state['redirect'] = 'admin/structure/forena/data';
  drupal_set_message(t('Deleted %s', array('%s' => $block_name)));
}

function forena_query_close($form, &$form_state) {
  Frx::BlockEditor($form_state['storage']['block_name'])->cancel();
  $form_state['rebuild'] = FALSE;
  $form_state['redirect'] = 'admin/structure/forena/data';

}


/**
 * Recursively clean all submit values from form elements
 * assuming tree = true structure.
 * @param $values
 */
function _forena_query_clean_values(&$values, &$elements) {
  foreach ($elements as $key => $value) if (strpos($key, '#')!==0) {
    if (@$value['#type']=='submit') {
      unset($values[$key]);
    }
    else {
      _forena_query_clean_values($values[$key], $elements[$key]);
    }
  }
}




/**
 * Function to allow all normal button submits to be for the form.
 * @param unknown_type $form
 * @param unknown_type $form_state
 */
function forena_query_editor_update($form, &$form_state) {
  // Instantiate form values
  $o = Frx::BlockEditor($form_state['storage']['block_name']);
  Frx::BlockEditor()->update($form_state['values']);
  $form_state['rebuild'] = TRUE;
}

function forena_query_update_builder($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $values = $form_state['values'];
  _forena_query_clean_values($values['where'], $form['where']);
  $form_state['storage']['builder']['where'] = $values['where'];
  $provider = $form_state['storage']['provider'];
  $sql = Frx::RepoMan()->buildSQL($provider, $form_state['storage']['builder']);
  $form_state['storage']['block']['file'] = $sql;
  $form_state['storage']['block']['builder'] = $form_state['storage']['builder'];
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
  $field = array_fill_keys(array('field', 'op', 'value'), '');
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
    $where['filter'][] =  array_fill_keys(array('field', 'op', 'value'), '');
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

/**
 * Build SQL query from data
 *
 */
function forena_query_build_sql($form, &$form_state) {
  $builder = $formstate['storage']['builder'];
  foreach ($builder as $filter) {

  }
}
