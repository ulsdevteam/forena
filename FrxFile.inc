<?php
/**
 * @file FrxFile.inc
 * File toolbox for manipulating files
 * contained tn the report directory.
 */
class FrxFile {
  public $dir; // Path to Default directory
  public $writable;
  public $includes = array(); //Other places to look for a directory.
  public $use_includes;
  public $allowed_extensions;

  public function __construct($default_directory, $include_directories, $extentions = array(), $use_includes=TRUE) {
    // Check to see if directory is writable
    $this->dir = $default_directory;
    $this->includes = $include_directories;
    $this->use_includes = $use_includes;
    $this->writable = is_writable($this->dir);
    $this->cached_extensions = $extentions;
  }

  private function scanDirectory($directory, &$files, $recursive=TRUE) {
    // Loop through the directories, ignoring hidden files.
    $path = $directory;
    // Scan the directory for files.
    $d = dir($path);
    if ($d) while (false !== ($rpt_file = $d->read())) {
      $src_file = trim($d->path, '/') . '/' . trim($rpt_file, '/');
      if (is_file($src_file)) {
        @list($base_file, $ext) = explode('.', $rpt_file, 2);
        if (array_search($ext, $this->cached_extensions) !== FALSE) {
          $files[$ext][$src_file] = $src_file;
        }
      }
      elseif (is_dir($src_file)) {
        if (strpos($rpt_file, '.')!==0 && $recursive) {
          $this->scanDirectory($src_file, $files, $recursive);
        }
      }
    }
    if ($d) $d->close();

  }

  public function scan($mask_dir='') {
    // Find all the directories that match this one.
    $files = array();
    $this->scanDirectory($this->dir, $files);
    foreach( $this->includes as $directory) {
      $this->scanDirectory($directory, $files);
    }
    return $files;
  }

  /**
   * Revert an individual report
   * @param $file
   */
  public function revert($file) {
    $i = 0;
    if ($this->includeExists($file)) {
      $file_to_delete = $this->dir . '/' . $file;
      if (file_exists($file_to_delete)) {
        if (is_writeable(dirname($file_to_delete))) {
          drupal_set_message(t('Removing customised report %s', array('%s' => $file_to_delete)));
          unlink($file_to_delete);
          $i++;
        }
        else {
          drupal_set_message(t('Unable to revert %s', array('%s' => $file_to_delete)), 'error');
        }
      }
    }
    return $i;
  }

  /**
   * Determine if the file exists in the include path.
   * @param $file
   */
  public function includeExists($file) {
    $found = false;
    $i = 0;
    while(isset($this->includes[$i]) && !$found ) {
      $filename = $this->includes[$i] . '/' . $file;
      if (file_exists($this->includes[$i] . '/' . $file)) {
        $found = TRUE;
      }
      $i++;
    }
    return $found;
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
    @list ($dir, $name_part) = explode('/', $filename, -1);
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

  function verifyDirectory($fullpath, $recursive=FALSE) {
    static $path='';
    $success = TRUE;
    if (!$recursive) {
      $path = $this->dir;
      if (!is_writable($path)) {
        drupal_set_message(t('Report directory %s is not modifiable', array('%s' => $path)), 'error');
        return FALSE;
      }
    }
    @list($dir, $file) = explode('/', $fullpath, 2);
    $path .= '/' . $dir;


    // Path
    if (!file_exists($path) && $file) {
      @mkdir($path);
      if (!@is_writable($path)) {
        drupal_set_message(t('Error creating directory %path', array('%path' => $path)), 'error');
        return FALSE;
      }

    }
    // Recurse to next file.
    if ($file && strpos($file, '/')) {
      $this->verifyDirectory($file, TRUE);
    }
    return TRUE;
  }

  /**
   * Save a file into the report directory.
   * @param unknown_type $filename
   * @param unknown_type $data
   */
  public function save($filename, $data) {
    $path = $this->dir . '/' . $filename;
    $this->verifyDirectory($filename);

    if (is_writable($path) || (!file_exists($path) && is_writable(dirname($path)))) {
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