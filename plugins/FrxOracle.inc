<?php
// $Id$
/**
 * @file
 * Oracle specific driver that takes advantage of oracles native XML support
 * 
 * In order to take advantage of XML support the following XML 
 *
 */
class FrxOracle extends FrxDataEngine {
  
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
    $uri = $conf['uri'];  
    if ($uri) { 
      try { 
        $db = oci_connect($conf['user'], $conf['password'], $uri, $conf['character_set']);
        $this->db = $db; 
      } catch (Exception $e) { 
        forena_error('Unable to connect to database', $e->getMessage());  
      }
   
    } 
    else { 
      forena_error('No database connection string specified'); 
    }
    
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
    $db = $this->db; 
    $filename = $this->block_path .'/'. $block_name .'.sql';  
    $block = forena_load_block_file($filename); 
    $xml =''; 
    if ($block['source'] && $this->access($block['access']) && $db) { 
      $sql = $block['source']; 
      $sql = $this->te->replace($sql, $params);

      $xml = new SimpleXMLElement('<table/>'); 

      //$rs->debugDumpParams();
      $stmt = oci_parse($db, $sql); 
      oci_execute($stmt);  

      while ($row = oci_fetch_array($stmt, OCI_ASSOC+OCI_RETURN_NULLS)) { 
        $row_node = $xml->addChild('row'); 
        foreach ($row as $key => $value) {
          $row_node->addChild(strtolower($key), $value); 
        }
      }
      oci_free_statement($stmt); 
      return $xml; 
    }
    
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
    $value =  "'".  str_replace("'", "''", $value) ."'"; 
    return $value; 
  }
  
  
}