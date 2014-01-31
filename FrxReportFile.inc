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
      $default_directory = variable_get('file_' . file_default_scheme() . '_path', conf_path() . '/files/reports');
    }
    $dir = rtrim($report_path, '/');
    $directories = module_invoke_all('forena_report_directory');
    foreach ($directories as $dir) {
      $this->includes[] = rtrim($dir, '/');
    }

    // Parent constructor.
    parent::__construct($dir, $directories);
  }
}