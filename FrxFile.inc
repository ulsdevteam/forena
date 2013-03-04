<?php
/**
 * @file FrxFile.inc
 * File toolbox for manipulating files
 * contained tn the report directory.
 */
class FrxFile {
  public $dir; // Path to directoy containing report paths.

  /**
   * Constructor
   *   Sets the initial reort directory
   */
  public function __construct() {
    $report_path = variable_get('forena_report_repos', '');
    if (!$report_path) {
      $report_path = variable_get('file_' . file_default_scheme() . '_path', conf_path() . '/files/reports');
    }
    $this->dir = rtrim($report_path, '/');
  }

  /**
   * Return the full path to the filename
   *   @param $filename
   */
  public function path($filename) {
    return $this->dir . '/' . $filename;
  }

  /**
   * Return the directory portioin of a report filename.
   * @param unknown_type $filename
   */
  public function directory($filename) {
    @list ($dir, $name_part) = explode('/', $filename, -2);
    if (!$name_part) {
      $name_part = $dir;
      $dir = '';
    }
    return $this->dir . '/' . $dir;
  }

  /**
   * Return whether the file exists.
   * @param unknown_type $filename
   */
  public function exists($filename) {
    return file_exists($this->dir . '/' . $filename);
  }

  /**
   * Return the contents of a file located in the report directory
   * @param $filename filename and extension for report file.
   */
  public function contents($filename) {
    if ($this->exists($filename)) {
      return file_get_contents($this->path($filename));
    }
    else {
      return '';
    }
  }



}