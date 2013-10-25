<?php
/**
 * @file FrxFile.inc
 * File toolbox for manipulating files
 * contained tn the report directory.
 */
class FrxFile {
  public $dir; // Path to directoy containing report paths.
  public $writable;
  public $includes = array(); //Other places to look for a directory.

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
    $directories = module_invoke_all('forena_report_directory');
    foreach ($directories as $dir) {
      $this->includes[] = rtrim($dir, '/');
    }
    // Check to see if directory is writable
    $this->writable = is_writable($this->dir);
  }

  /**
   * Return the full path to the filename
   *   @param $filename
   */
  public function path($filename, $use_include = TRUE) {
    $path = $this->dir . '/' . $filename;
    if ($use_include && !file_exists($path)) {
      foreach ($this->includes as $dir) {
        if (file_exists($dir . '/' . $filename)) {
          $path = $dir . '/' . $filename;
        }
      }
    }
    return $path;
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
  public function exists($filename, $use_include = TRUE) {
    return file_exists($this->path($filename, $use_include));
  }

  /**
   * Return the contents of a file located in the report directory
   * @param $filename filename and extension for report file.
   */
  public function contents($filename) {
    $path = $this->path($filename);
    if (file_exists($path)) {
      return file_get_contents($path);
    }
    else {
      return '';
    }
  }

  /**
   * Save a file into the report directory.
   * @param unknown_type $filename
   * @param unknown_type $data
   */
  public function save($filename, $data) {
    $path = $this->dir . '/' . $filename;
    if (is_writable($path)) {
      file_put_contents($path, $data);
    }
    else {
      Frx::error(t('Insufficient privileges to write file.'));
    }
  }

  /**
   * Delete a file from the directory.
   * @param unknown_type $filename
   * @return boolean
   */
  public function delete($filename) {
    $filepath = $this->path($filename, FALSE);
    $info = pathinfo($filepath);
    $do = TRUE;
    if (file_exists($filepath)) {
      chdir($info['dirname']);
      if ($delete_file) $do = unlink($info['basename']);
    }
    return $do;
  }

  /**
   * Retrieve path info
   * @param $filename filename used for data
   * @param $use_include boolean value determining whether to search include path.
   * @return mixed
   */
  public function pathinfo($filename, $use_include = TRUE) {
    return pathinfo($this->path($filename, $use_include));
  }



}