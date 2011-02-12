<?php
// $Id$
/**
 * @file
 * General database engine used to do sql queries. 
 *
 */

class FrxPDO extends FrxDataProvider { 
  

  private $db;
  public $debug; 
 
  
  /**
   * Object constructor
   *
   * @param unknown_type $uri Database connection string.
   * @param string $repos_path Path to location of data block definitions
   */
  public function __construct($conf, $repos_path) {
    parent::__construct($conf, $repos_path);
    $uri = $conf['uri']; 
    $this->debug = $conf['debug'];  
    if ($uri) { 
      // Test for PDO suport
      if (!class_exists('PDO')) { 
        forena_error('PDO support not installed.', 'PDO support not installed.'); 
        return;
      }
      
      // Test for driver support
      list($prot, $c) = explode(':', $uri, 2); 
      $drivers = PDO::getAvailableDrivers(); 
      if ($drivers && (array_search($prot, $drivers)===FALSE)) {
      	$msg = 'PDO driver support for ' . $prot . ' not installed'; 
      	forena_error($msg, $msg);
      	return; 
      }
      try { 
      	if ($conf['user']) { 
          $db = new PDO($uri, $conf['user'], $conf['password']);
      	}
      	else {
      		$db = new PDO($uri); 
      	}
        $this->db = $db; 
        if (!is_object($db)) { 
        	forena_error('Unknown error connecting to database '. $uri); 
        }
      } catch (PDOException $e) { 
        forena_error('Unable to connect to database', $e->getMessage());  
      }
           
    } 
    else { 
      forena_error('No database connection string specified'); 
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
	        $sql = 'SELECT * FROM ('. trim($sql, ' ;') . ') forena_table '. $clause; 
	      }
      $sql = $this->te->replace($sql, $params); 
      $rs = $db->query($sql); 
      $xml = new SimpleXMLElement('<table/>'); 
      
      $e = $db->errorCode();
      if ($e != '00000') {
       $i = $db->errorInfo(); 
       forena_error($i[2] .':'. $sql, $i[2]); 
        
      } 
      else { 

        //$rs->debugDumpParams(); 
        $data = $rs->fetchAll(PDO::FETCH_ASSOC); 
        foreach ($data as $row) { 
          $row_node = $xml->addChild('row'); 
          foreach ($row as $key => $value) {
            $row_node->addChild($key, htmlspecialchars($value)); 
          }
        }
        
      }
      
      if ($this->debug) { 
        if ($xml) $d = htmlspecialchars($xml->asXML); 
        forena_debug('SQL: '. $sql, '<pre> SQL:'. $sql . "\n XML: ". $d . "/n</pre>"); 
      }      
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
    $db = $this->db; 
    if ($db) {
      if ($value=='') $value = 'NULL'; else  $value =  $db -> quote($value); 
    }
    return $value; 
  }
  
}