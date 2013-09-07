<?php
/**
 * @file forena.drush.inc
 * Implementation of drush command hook.
 */
function forena_drush_command() {
  $items = array();

  $items['forena-clear-cache'] = array(
    'description' => 'Forena clear report cache',
    'examples' => array('drush frxcc'),
    'aliases' => array('frxcc')
  );

  $items['forena-deliver-reports'] = array(
    'description' => 'Forena Copy/Deliver module provided forena reports',
    'examples' => array('drush frxcp'),
    'aliases' => array('frxcp'),
  );
  return $items;
}

/**
 * Execute a clear forena's cache from drush
 */
function drush_forena_clear_cache() {
  require_once 'forena.common.inc';
  require_once 'forena.admin.inc';
  forena_sync_reports(FALSE);
}

/**
 * Execute copy of the reports from drush.
 */
function drush_forena_deliver_reports() {
  require_once 'forena.common.inc';
  require_once 'forena.admin.inc';
  forena_sync_reports(TRUE);
}