<?php
/**
 * @file forena.drush.inc
 * Implementation of drush command hook.
 */
function forena_drush_command() {
  $items = array();

  $items['clear-forena-cache'] = array(
    'description' => 'Clear forenas report cache',
    'examples' => array('drush frxcc'),
    'aliases' => array('frxcc')
  );

  $items['deliver-forena-reports'] = array(
    'description' => 'Copy/Deliver module provided forena reports',
    'examples' => array('drush frxcp'),
    'aliases' => array('frxcp'),
  );
  return $items;
}

/**
 * Execute a clear forena's cache from drush
 */
function drush_forena_clear_forena_cache() {
  require_once 'forena.common.inc';
  require_once 'forena.admin.inc';
  forena_sync_reports(FALSE);
}

/**
 * Execute copy of the reports from drush.
 */
function drush_forena_deliver_forena_reports() {
  require_once 'forena.common.inc';
  require_once 'forena.admin.inc';
  forena_sync_reports(TRUE);
}