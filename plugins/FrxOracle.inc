<?php
// $Id$
/**
 * @file
 * Oracle specific driver that takes advantage of oracles native XML support
 * 
 * In order to take advantage of XML support the following XML 
 *
 */
class FrxOracle extends FrxDataProvider {
  

  private $db;
  private $use_oracle_xml; 
  
  /**
   * Object constructor
   *
   * @param unknown_type $uri Database connection string.
   * @param string $repos_path Path to location of data block definitions
   */
  public function __construct($conf, $repos_path) {
    parent::__construct($conf, $repos_path);
    $this->use_oracle_xml = FALSE;
    $uri = $conf['uri'];  
    $this->debug = $conf['debug']; 
    if ($conf['oracle_xml']) $this->use_oracle_xml = TRUE; 
    if ($uri) { 
      try { 
        $db = oci_connect($conf['user'], $conf['password'], $uri, $conf['character_set']);
        $this->db = $db; 
      } catch (Exception $e) { 
        forena_error('Unable to connect to database '. $conf['title'], $e->getMessage());  
      }
   
    } 
    else { 
      forena_error('No database connection string specified', 'No database connection: '. print_r($conf,1)); 
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
  public function data($block_name, $params=array(), $clause='') {
    // Load the block from the file
    $db = $this->db; 
    $block = $this->load_block($block_name); 
    $xml =''; 
    if ($block['source'] && $this->access($block['access']) && $db) { 
      $sql = $block['source']; 
      if ($clause) {
        $sql = 'SELECT * FROM ('. trim($sql, ' ;') . ') '. $clause; 
      }
      $sql = $this->te->replace($sql, $params);

      if ($this->use_oracle_xml) { 
        $xml = $this->oracle_xml($sql); 
      } 
      else {
        $xml = $this->php_xml($sql); 
      }
      if ($this->debug) { 
        if ($xml) $d = htmlspecialchars($xml->asXML); 
        forena_debug('SQL: '. $sql, '<pre> SQL:'. $sql . "\n XML: ". $d . "\n</pre>"); 
      }
      return $xml; 
    }
    
  }
  
  /**
   * Generate xml from sql using the provided f_forena
   *
   * @param unknown_type $sql
   * @return unknown
   */
  private function oracle_xml($sql) { 
      $db = $this->db; 

      //$rs->debugDumpParams();
      $fsql = 'declare x XMLTYPE; begin x := f_forena_xml(:p1); :ret_val := x.getClobVal();  end; '; 
      $stmt = oci_parse($db, $fsql);
      $ret = oci_new_descriptor($db, OCI_D_LOB); 
      oci_bind_by_name($stmt, ':ret_val', $ret, -1, OCI_B_CLOB);  
      oci_bind_by_name($stmt, ':p1', $sql);
      oci_execute($stmt);  
      $xml_text = $ret->load(); 
      if ($xml_text) $xml = new SimpleXMLElement($xml_text);  
      oci_free_statement($stmt); 
      return $xml;     
  }
  
  private function php_xml($sql) { 
      $db = $this->db; 
      $xml = new SimpleXMLElement('<table/>'); 

      //$rs->debugDumpParams();
      $stmt = oci_parse($db, $sql); 
      oci_execute($stmt);  

      while ($row = oci_fetch_array($stmt, OCI_ASSOC+OCI_RETURN_NULLS)) { 
        $row_node = $xml->addChild('row'); 
        foreach ($row as $key => $value) {
          $row_node->addChild(strtolower($key), htmlspecialchars($value)); 
        }
      }
      oci_free_statement($stmt); 
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
    if ($value=='') $value = 'NULL'; else $value =  "'".  str_replace("'", "''", $value) ."'"; 
    return $value; 
  }
  
    /**
   * Destructor - Closes database connections. 
   *
   */
  public function __destruct() { 
    $db = $this->db; 
    if ($db) { 
      oci_close($db); 
    }
  }
  
  
  
}