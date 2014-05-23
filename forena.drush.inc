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
  $items['forena-report'] = array(
    'description' => 'Render a report',
    'arguments' => array(
  	  'report_uri' => 'The name of the report to render.  Include parameters as url parameters to the report as you would see them on the url',
      'filename' => 'The file to write the report data to',
    ),
    'options' => array('language'),
    'aliases' => array('frx'),
  	'examples' => array(
  	  'drush forena-report sample/states.html states.html' => 'Export an html report as a simple table',
  	  "drush forena-report 'sample/user_distribution_simple.xml?state=wa wa.xml'" => 'Export the states as an xml for washington sate',
    ),
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

/**
 * Run a report to a fiel.
 */
function drush_forena_report($report_uri, $filename='') {
  require_once 'forena.common.inc';
  $parms = array();
  $query = '';
  $report_name = $report_uri;
  @list($report_name, $query) = explode('?', $report_uri);
  if ($query) {
    parse_str($query, $parms);
  }
  if ($filename) {
    drupal_set_message(t('Writing report %s  to %f.', array('%s' => $report_name, '%f' => $filename)));
    file_put_contents($filename, forena_report($report_name, $parms, FALSE));
  }
  else {
    drupal_set_message(t('Generating report %s.', array('%s' => $report_name)));
    print forena_report($report_name, $parms, FALSE);
  }
}