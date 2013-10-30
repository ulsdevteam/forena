<?php
/**
 * @file
 * General database engine used to do sql queries.
 *
 */

class FrxPDO extends FrxDataSource {


  private $db;
  public $debug;


  /**
   * Object constructor
   *
   * @param unknown_type $uri Database connection string.
   * @param string $repos_path Path to location of data block definitions
   */
  public function __construct($conf, $repos_path, $name) {
    parent::__construct($conf, $repos_path, $name);
    $uri = $conf['uri'];
    $this->debug = $conf['debug'];
    if ($uri) {
      // Test for PDO suport
      if (!class_exists('PDO')) {
        FrxReportGenerator::instance()->error('PDO support not installed.', 'PDO support not installed.');
        return;
      }

      $options = array();
      if (@$conf['mysql_charset']) {
        $options = array(
        PDO::MYSQL_ATTR_INIT_COMMAND => 'SET NAMES ' . $conf['mysql_charset'],
        );
      }

      // Test for driver support
      @list($prot, $c) = explode(':', $uri, 2);
      $drivers = PDO::getAvailableDrivers();
      if ($drivers && (array_search($prot, $drivers)===FALSE)) {
        $msg = 'PDO driver support for ' . $prot . ' not installed';
        FrxReportGenerator::instance()->error($msg, $msg);
        return;
      }
      try {
        if (isset($conf['user'])) {
          $db = new PDO($uri, $conf['user'], @$conf['password'], $options);
        }
        else {
          $db = new PDO($uri, NULL, NULL, $options);
        }
        $this->db = $db;
        if (!is_object($db)) {
          FrxReportGenerator::instance()->error('Unknown error connecting to database ' . $uri);
        }
      } catch (PDOException $e) {
        FrxReportGenerator::instance()->error('Unable to connect to database', $e->getMessage());
      }

    }
    else {
      FrxReportGenerator::instance()->error('No database connection string specified');
    }

    // Set up the stuff required to translate.
    $this->te = new FrxSyntaxEngine(FRX_SQL_TOKEN, ':', $this);
  }
  /**
   * Get data based on file data block in the repository.
   *
   * @param String $block_name
   * @param Array $parm_data
   * @param Query $subQuery
   */
  public function sqlData($sql, $options = array()) {
    // Load the block from the file
    $db = $this->db;
    $xml ='';
    // Load the types array based on data
    $this->types = isset($options['type']) ? $options['type'] : array();

    if ($sql && $db) {
      $sql = $this->te->replace($sql);
      $rs = $db->query($sql);
      $xml = new SimpleXMLElement('<table/>');
      $e = $db->errorCode();
      if ($e != '00000') {
        $i = $db->errorInfo();
        FrxReportGenerator::instance()->error($i[2] . ':' . $sql, $i[2]);
      }
      else if ($rs && $rs->columnCount()) {

        $data = $rs->fetchAll(PDO::FETCH_ASSOC);
        $rownum = 0;
        if ($data) foreach ($data as $row) {
          $rownum++;
          $row_node = $xml->addChild('row');
          $row_node['num'] = $rownum;
          foreach ($row as $key => $value) {
            $row_node->addChild($key, htmlspecialchars($value));
          }
        }
      }

      if ($this->debug) {
        if ($xml) $d = htmlspecialchars($xml->asXML);
        FrxReportGenerator::instance()->debug('SQL: ' . $sql, '<pre> SQL:' . $sql . "\n XML: " . $d . "/n</pre>");
      }
      return $xml;
    }

  }

  /**
   * Wrapper method cause some ODBC providers do not support
   * quoting.   We're going to assume the MSSQL method of quoting.
   * @param $value
   */
  public function quote($value) {
    $new_value =  $this->db->quote($value);
    if (($value!=='' || $value!==NULL) && !$new_value) {
      $value = "'" . str_replace("'", "''", $value) . "'";
    }
    else {
      $value = $new_value;
    }
    return $value;
  }

  /**
   * Implement custom SQL formatter to make sure that strings are properly escaped.
   * Ideally we'd replace this with something that handles prepared statements, but it
   * wouldn't work for
   *
   * @param unknown_type $value
   * @param unknown_type $key
   * @param unknown_type $data
   */
  public function format($value, $key) {
    $db = $this->db;
    $value = $this->parmConvert($key, $value);
    if ($db) {
      if ($value==='' || $value ===NULL || $value === array()) {
        $value = 'NULL';
      }
      elseif (is_int($value)) {
        $value = (int)$value;
        $value = (string)$value;
      }
      elseif (is_float($value)) {
        $value = (float)$value;
        $value = (string)$value;
      }
      elseif (is_array($value)) {
        if ($value == array()) {
          $value = 'NULL';
        }
        else {
          // Build a array of values string
          $i=0;
          $val ='';
          foreach ($value as $v) {
            $i++;
            if ($i!=1) {
              $val .= ',';
            }
            $val .= $this->quote($v);
          }
          $value = $val;
        }
      }
      else  $value =  $this->quote($value);
    }
    return (string)$value;
  }

}