<?php
/**
 * @file forena.drush.inc
 * Implementation of drush command hook.
 */
function forena_drush_command() {
  $items = array();

  $items['forena-deliver-reports'] = array(
    'description' => 'Forena Revert Delivered module provided forena reports',
    'examples' => array('drush frxrevert'),
    'aliases' => array('frxcp', 'frxrevert'),
  );
  return $items;
}


/**
 * Execute copy of the reports from drush.
 */
function drush_forena_deliver_reports() {
  require_once 'forena.common.inc';
  require_once 'forena.admin.inc';
  forena_sync_reports(TRUE);
}