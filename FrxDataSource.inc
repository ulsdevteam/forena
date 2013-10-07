<?php
// $Id$
require_once('forena.common.inc');
/**
 * @file
 * Class that defines default methods for access control in an FrxDataSource
 *
 */
class FrxDataSource{
  public $name;
  public $conf;
  public $block_path;
  public $comment_prefix;
  public $comment_suffix;
  public $block_ext;
  public $block_extensions;
  public $types;
  public $block_name;
  protected $te;
  public $debug = FALSE;
  public function __construct($conf, $repos_path, $name) {
    $this->conf = $conf;
    $this->comment_prefix = '--';
    $this->block_ext = 'sql';
    $this->block_path = $repos_path;
    $this->app = FrxReportGenerator::instance()->app;
    $this->block_extensions = array('inc', 'sql', 'xml');
    $this->name = $name;
    $this->debug = @$conf['debug'];
  }

  /**
   * Implements the basic default security check of calling
   * an access method.
   *
   * @param unknown_type $arg
   * @return unknown
   */
  public function access($arg) {
    $f = @$this->conf['access callback'];
    if ($f && is_callable($f)) {
      return $f($arg);
    }
    elseif (isset($this->conf['access block'])) {
      $block = @$this->conf['access block'];
      $path='';
      if (isset($this->conf['access path'])) $path = $this->conf['access path'];
      return FrxReportGenerator::instance()->block_access($block, $path, $arg);
    }
    else {
      return user_access('access content');
    }
  }

  protected function loadBlockFromDB($block_name) {
    $block = array();
    // IF load from the database if forena_query is there.
    if (module_exists('forena_query')) {

      // See if we have one in the database first
      $sql = 'SELECT * FROM {forena_data_blocks} WHERE repository=:repos AND block_name = :block_name';
      $rs = db_query(
      $sql,
      array(
          ':repos' => $this->name,
          ':block_name' => $block_name,
      )
      );

      if ($b = $rs->fetchObject()) {
        $block = array(
          'repository' => $b->repository,
          'block_name' => $b->block_name,
          'type' => $b->block_type,
          'file' => $b->src,
          'builder' => $b->builder,
          'access' => $b->access,
          'title' => $b->title,
          'locked' => $b->locked,
          'modified' => $b->modified,
        );
        switch ($block['type']) {
          case 'sql':
            $block = array_merge($block, $this->parseSQLFile($block['file']));
            break;
          case 'xml':
            $block = array_merge($block, $this->parseXMLFile($block['file']));
            break;
        }
        if ($block['builder']) {
          $block['builder'] = unserialize($block['builder']);
        }

      }
    }
    return $block;
  }

  protected function loadBlockFromFile($block_name) {
    $base_file = $this->block_path . '/' . $block_name;
    $parts = @explode('.', $block_name);
    $php_method = array_pop($parts);
    $php_class = implode('.', $parts);
    $php_file = $this->block_path . '/' . $php_class . '.inc';
    if (file_exists($base_file . '.sql')) {
      $contents = file_get_contents($base_file . '.sql');
      $block = $this->parseSQLFile($contents);
      $block['type'] = 'sql';
    }
    elseif (file_exists($base_file . '.xml')) {
      $contents = file_get_contents($base_file . '.xml');
      $block = $this->parseXMLFile($contents);
      $block['type'] = 'xml';
    }
    elseif (file_exists($php_file)) {
      require_once $php_file;
      if (class_exists($php_class)) {
        $o = new $php_class();
        $block['type'] = 'php';
        $block['access'] = @$o->access;
        $block['object'] = $o;
        $block['method'] = $php_method;
        if (method_exists($o, 'tokens' )) {
          $block['tokens'] = $o->tokens();
        }
        elseif (isset($o->tokens)) {
          $block['tokens'] = $o->tokens;
        }
        else{
          $block['tokens'] = array();
        }
      }
    }
    else {
      return array();
    }
    $block['locked']=1;
    return $block;
  }


  /**
   * Load blcok data from filesystem
   * @param $block_name
   */
  function loadBlock($block_name, $include=FALSE) {

    $block = array();
    if ($include) $this->block_name = $block_name;
    $block = $this->loadBlockFromDB($block_name);
    if (!$block) {
      $block = $this->loadBlockFromFile($block_name);
    }
    return $block;
  }


  /**
   * Load tokens from block source
   */
  public function tokens($source) {
    $tokens = array();
    // If we have a regular expression token parser, then get the tokens out of the block.
    if ($this->te) {
      $tokens = @$this->te->tokens($source);
      $tokens = array_diff($tokens, array('current_user'));
      //check tokens in the where clause
    }

    return $tokens;
  }

  public function listDBBlocks($search, &$data_blocks) {
    $search = '%' . $search . '%';
    $sql = 'SELECT * from {forena_data_blocks} WHERE repository=:repos
      AND block_name like :search ';
    $rs = db_query($sql, array(':repos' => $this->name, ':search' => $search ));
    foreach ($rs as $block) {
      $data_blocks[] = $block->block_name;
    }
  }


  /**
   * Find all the blocks matching a provided search string
   *
   * @param string $search part block names to search for
   * @return unknown
   */
  public function list_blocks($search, &$block_list, $subdir='') {
    static $count=0;
    // First find files that match the search string
    if ($count > 100) return;
    $path = $this->block_path . '/';
    if ($subdir) $path .= $subdir . '/';
    $block_path = $path . '*' . $search . '*';
    // Find sql files

    $d = glob($block_path);
    if ($d) foreach ($d as $file_name) {
      // Split off the extention
      $p = strripos($file_name, '.');
      if ($p!==FALSE) {
        $ext = substr($file_name, $p+1);
        $block_name = substr($file_name, 0, $p);
      }
      else {
        $ext = '';
        $block_name = $file_name;
      }
      switch ($ext) {
        case 'inc':
          require_once $file_name;
          $class = str_replace($this->block_path . '/', '', $block_name);
          $methods = get_class_methods($class);
          if ($methods) foreach ($methods as $method) {
            if ($method != 'tokens') {
              $block_list[] = $class . '.' . $method;
            }
          }

          break;
        default:
          if (array_search($ext, $this->block_extensions)!==FALSE) {
            $block_list[] = str_replace($this->block_path . '/', '', $block_name);

          }
      }
    }
    $count++;
    // Find directories
    $d = glob($path . '*');
    if ($d) foreach ($d as $dir_name) {
      if (is_dir($dir_name)) {
        $dir_name = str_replace($this->block_path . '/', '', $dir_name);
        $this->list_blocks($search, $block_list, $dir_name);
      }
    }
    // Date
    if (!$subdir && module_exists('forena_query'))  {
      $this->listDBBlocks($search, $block_list);
    }
  }

  public function debug($msg='', $log='') {
    FrxReportGenerator::instance()->app->debug($msg, $log);
  }

  public function error($msg='', $log='') {
    if ($msg) {
      drupal_set_message(check_markup($msg), 'error');
    }
    if ($log) {
      watchdog('forena', $log, NULL, WATCHDOG_ERROR);
    }
  }

  public function parseXMLFile($contents) {
    $comment = $this->comment_prefix;
    $trim = '->';
    $lines = explode("\n", $contents);
    $cnt = count($lines);
    $access = '';
    $i=0;
    $block = '';
    $data = '';
    while ($i<$cnt) {
      $l = trim($lines[$i], "\r");
      @list($d, $c) = explode($comment, $l, 2);
      if ($trim) $c = trim($c, $trim);
      if  ($c) {
        list($a, $o) = explode('=', $c, 2);
        $a = trim($a);
        if ($a && $o) {
          switch ($a) {
            case 'ACCESS':
              $access = trim($o);
              break;
            default:
          }

        }

      }
      if (strpos($l, $comment)!==0) {
        $data .= "$l\n";
      }
      $i++;
    }
    return array('access' => $access, 'source' => $data, 'tokens' => $this->tokens($data));
  }

  public function getSQLInclude($block_name) {
    //@TODO: allow relative block includes
    $block = $this->loadBlock($block_name, TRUE);
    if ($block['type'] == 'sql') {
      return $block;
    }
  }


  public function parseSQLFile($contents) {
    $comment = $this->comment_prefix;
    $trim = $this->comment_suffix;
    $lines = explode("\n", $contents);
    $cnt = count($lines);
    $access = '';
    $i=0;
    $block = '';
    $parms = Frx::Data()->currentContext();
    $data = '';
    $source = '';
    $file = '';
    $skip = FALSE;
    $in_info = FALSE;
    $info_text = '';
    $tokens = array();
    $options = array();
    while ($i<$cnt) {
      $l = trim($lines[$i], "\r");
      @list($d, $c) = explode($comment, $l, 2);
      if ($trim) $c = trim($c, $trim);
      if  ($c) {
        $c = trim($c);
        @list($a, $o) = explode('=', $c, 2);
        $a = trim($a);
        if (($a && $o) || $c == 'END' || $c == 'ELSE' || $c == 'INFO') {
          if ($c != 'INFO' ) {
            $in_info = false;
          }
          switch ($a) {
            case 'ACCESS':
              $access = trim($o);
              break;
            case 'IF':
              $skip = !$this->te->test(trim($o));
              break;
            case 'END':
              $skip = FALSE;
              break;
            case 'ELSE':
              $skip = !$skip;
              break;
            case 'INFO':
              $in_info = TRUE;
              break;
            case 'INCLUDE':
              $inc_block = $this->getSQLInclude(trim($o));
              if ($inc_block) {
                $data .= $inc_block['source'];
                $tokens = array_merge($tokens, $inc_block['tokens']);
              }
              break;
          }

        }
        if ($a != 'ACCESS') $file .= "$l\n";

      }
      else {

        $file .= "$l\n";
      }
      if ($in_info) {
        if (strpos($l, $comment)!==0 && $l) $info_text .= "$l\n";
      } elseif (!$skip) {
        if (strpos($l, $comment)!==0 && $l) {
          $data .= "$l\n";
        }
      }

      $i++;
    }
    $tokens = array_merge($tokens, $this->tokens($contents));
    $options = drupal_parse_info_format($info_text);

    $block = array( 'source' => $data, 'file' => trim($file, " \n"),
      'tokens' => $tokens, 'options' => $options);
    if ($access) $block['access'] = $access;
    return $block;
  }

  /**
   * Dummy method for returning sql data
   * @param $parameters array Block parameters
   * @param $block array extra block options for data provider.
   */
  public function sqlData($parameters, $options= array()) {
    return '';
  }

  /**
   * Implement static XML functioin
   * @param $source XML Source data from block load
   * @param $parm_data Parameter data
   */
  public function xmlData($source, $parm_data='') {
    $xml ='';
    try {
      $xmlData = $source;
      $xml = new SimpleXMLElement($xmlData);
    } catch (Exception $e) {
      $this->error("Error processing xml\n", $e->getMessage() . "\n" . $xmlData);
    }
    return $xml;
  }

  public function phpData($o, $method, $parameters) {
    $data = NULL;
    if (is_object($o) && is_callable(array($o, $method))) {
      $data = $o->$method($parameters);
    }
    return $data;
  }

  /**
   * Save a data block
   * @param $block_name String name of block to save
   * @param $data array fields of block to save.
   */
  public function saveBlock($block_name, $data) {
    $modified = time();
    $builder = @$data['builder'];
    if (is_array($builder)) {
      $builder = serialize($builder);
    }

    $rs = db_query('SELECT * from {forena_data_blocks} WHERE repository=:repos AND
      block_name = :block_name',
    array(
        ':repos' => $this->name,
        ':block_name' => $block_name,
    )
    );
    if ($block = $rs->fetchObject()) {
      db_update('forena_data_blocks')
      ->fields(array(
          'src' => $data['file'],
          'builder' => $builder,
          'access' => $data['access'],
          'modified' => $modified,
      ))
      ->condition('repository', $this->name)
      ->condition('block_name', $block_name)
      ->execute();
    }
    else {
      db_insert('forena_data_blocks')
      ->fields( array(
          'repository' => $this->name,
          'block_name' => $block_name,
          'block_type' => $data['type'],
          'src' => $data['file'],
          'builder' => $builder,
          'access' => $data['access'],
          'locked' => 0,
          'modified' => $modified,
      ))
      ->execute();
    }
  }

  /**
   *
   * Delete data block stored in database.
   * @param $block_name Data block to delete
   */
  public function deleteBlock($block_name) {
    db_delete('forena_data_blocks')
    ->condition('repository', $this->name)
    ->condition('block_name', $block_name)
    ->execute();
  }

  /**
   * Build the SQL clause based on builder data.
   * @param  $data
   */
  public function buildFilterSQL($data) {
    $clause = '';
    $op = $data['op'];
    $i=0;
    if ($data['filter']) foreach ($data['filter'] as $cond) {
      $i++;
      $conj = ($i==1) ? '' : $op . ' ';
      if (isset($cond['filter'])) {
        $clause .= $conj . ' (' . $this->buildFilterSQL($cond) . " )\n";
      }
      else {

        switch ($cond['op']) {
          case 'IS NULL':
          case 'IS NOT NULL':
            $expr = $cond['field'] . ' ' . $cond['op'];
            break;
          default:
            $expr = $cond['field'] . ' ' . $cond['op'] . ' ' . $this->format($cond['value'], $cond['field'], array());
        }
        $clause .= ' ' . $conj . $expr;
      }
    }
    return $clause;
  }

  /**
   * Perform generic type conversion based on attributes.
   * @param  $key The token key for parameter replacement.
   * @param  $value The value of the parameter
   * @return Ambigous <NULL, array>
   */
  public function parmConvert($key, $value) {
    if (isset($this->types[$key]) && $this->types[$key]) {
      if ($value === NULL || $value ==='') {
        $value = NULL;
      }
      else {
        switch (strtolower($this->types[$key])) {
          case 'date':
            $time = @new DateTime($value);
            if ($time) {
              $value   = date_format($time, 'Y-m-d H:i:s');
            }
            else {
              $value = NULL;
            }
            break;
          case 'unixtime':
            $time = @new DateTime($value);
            if ($time) {
              $value   = $time->getTimeStamp();
            }
            else {
              $value = NULL;
            }
            break;
          case 'numeric':
          case 'float':
            $value = (float)$value;
            break;
          case 'int':
          case 'integer':
            $value = (int)$value;
            break;
          case 'array':
            $value = (array)$value;
            break;
        }
      }
    }
    return $value;
  }
}


