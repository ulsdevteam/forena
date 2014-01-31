<?php
require_once 'FrxFile.inc';
class FrxReportFile extends FrxFile {
  /**
   * Constructor
   *   Sets the initial reort directory
   */
  public function __construct() {
    // Load default directory from configuration.
    $report_path = variable_get('forena_report_repos', '');
    if (!$report_path) {
      $report_path = variable_get('file_' . file_default_scheme() . '_path', conf_path() . '/files/reports');
    }
    $default_directory = rtrim($report_path, '/');
    $directories = module_invoke_all('forena_report_directory');
    foreach ($directories as $dir) {
      $this->includes[] = rtrim($dir, '/');
    }

    // Parent constructor.
    parent::__construct($default_directory, $directories, array('frx', 'skinfo', 'css', 'js'));
  }
}