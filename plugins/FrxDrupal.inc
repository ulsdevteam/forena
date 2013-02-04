<?php
/**
 * @file
 * Provides data blocks for native drupal connections using the default
 * drupal connections.
 *
 */
class FrxDrupal extends FrxDataSource {
  /**
   * Implements hooks into the drupal applications
   */

  private $db;

  /**
   * Object constructor
   *
   * @param unknown_type $uri Database connection string.
   * @param string $repos_path Path to location of data block definitions
   */
  public function __construct($conf, $repos_path, $name) {
    parent::__construct($conf, $repos_path, $name);

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
  public function sqlData($sql) {
    // Load the block from the file
    $xml ='';
    $sql = $this->te->replace($sql);
    $rs = db_query($sql);

    $xml = new SimpleXMLElement('<table/>');
    foreach ($rs as $data) {
        $row_node = $xml->addChild('row');

        foreach ($data as $key => $value) {
          $row_node->addChild($key, htmlspecialchars($value));
          //$row_node->$key = $value;
        }
    }
    return $xml;
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
  public function format($value, $key, $data) {
    $db = Database::getConnection('default');
    if ($db) {
      if ($value=='')
        $value = 'NULL';
      else {
        if (is_array($value)) {
          if ($value == array()) {
            $value = 'NULL';
          }
          else {
            // Build a array of values string
            $i=0;
            $val = '';
            foreach ($value as $v) {
              $i++;
              if ($i>1) {
                $val .= ',';
               }
            $val .=   $db->quote($v);
            }
            $value = $val;
          }
        }
        else {
          $value = trim($value);
          $value =    $db->quote($value) ;
        }
      }
    }
    return $value;
  }

}













