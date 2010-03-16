<?php
/**
 * General database engine used to do sql queries. 
 *
 */

class FrxDBEngine extends FrxDataEngine { 
  
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
        $db = new PDO($uri, $conf['user'], $conf['password']);
        $this->db = $db; 
      } catch (PDOException $e) { 
        forena_error('Unable to connect to database', $e->getMessage());  
      }
   
    } else { 
      forena_error('No database connection string specified'); 
    }
    
    // Set up the stuff required to translate. 
    $this->block_path = $repos_path; 
    $this->te = new FrxSyntaxEngine(FRX_SQL_TOKEN,':',$this); 
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
    $filename = $this->block_path .'/'. $block_name . '.sql';  
    $block = forena_load_block_file($filename); 
    $xml =''; 
    if ($block['source'] && $this->access($block['access']) && $db) { 
      $sql = $block['source']; 
      $sql = $this->te->replace($sql,$params);
      $rs = $db->query($sql); 
      $xml = new SimpleXMLElement('<table/>'); 
      
      $e = $db->errorCode();
      if ($e!='00000') {
       $i = $db->errorInfo(); 
       forena_error($i[2].':'.$sql, $i[2]); 
        
      } else { 

        //$rs->debugDumpParams(); 
        $data = $rs->fetchAll(PDO::FETCH_ASSOC); 
        foreach ($data as $row) { 
          $row_node = $xml->addChild('row'); 
          foreach ($row as $key=>$value) {
            $row_node->addChild($key,$value); 
          }
        }
        
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
      $value =  $db -> quote($value); 
    }
    return $value; 
  }
  
}