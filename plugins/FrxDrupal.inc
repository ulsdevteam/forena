<?php
// $Id$
/**
 * @file
 * Provides data blocks for native drupal connections using the default 
 * drupal connections.
 *
 */
class frxDrupal extends FrxDataEngine {
  /**
	 * Implements hooks into the drupal applications
	 */
  private $te;  
  private $db;
  private $block_path; 
  
  /**
   * Object constructor
   *
   * @param unknown_type $uri Database connection string.
   * @param string $repos_path Path to location of data block definitions
   */
  public function __construct($conf, $repos_path) {
    parent::__construct($conf, $repos_path);
     
    // Set up the stuff required to translate. 
    $this->block_path = $repos_path; 
    $this->te = new FrxSyntaxEngine(FRX_SQL_TOKEN, ':', $this); 
  }
  /**
   * Get data based on file data block in the repository. 
   *
   * @param String $block_name
   * @param Array $parm_data
   * @param Query $subQuery
   */
  public function data($block_name, $params=array(), $subQuery='') {
    // Load the block from the file

    $filename = $this->block_path .'/'. $block_name . '.sql';  
    $block = forena_load_block_file($filename); 
    $xml =''; 
    if ($block['source'] && $this->access($block['access'])) { 
      $sql = $block['source']; 
      $sql = $this->te->replace($sql, $params);
      $rs = db_query($sql); 
      $xml = new SimpleXMLElement('<table/>'); 
      while ($data = db_fetch_object($rs)) { 

          $row_node = $xml->addChild('row'); 
          
          foreach ($data as $key => $value) {
            $row_node->addChild($key, htmlspecialchars($value)); 
            //$row_node->$key = $value; 
          }
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
    $value =  "'". db_escape_string($value) ."'"; 
    return $value; 
  }
  
}