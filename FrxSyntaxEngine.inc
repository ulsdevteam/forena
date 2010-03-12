<?php
define('FRX_TOKEN_EXP','/\{[^}]+}/'); 
define('FRX_SQL_TOKEN','/:([a-z]|[0-9]|{A-Z]|[-_])+/'); 
class FrxSyntaxEngine { 
  private $tpattern; 
  private $trim_chars; 
  private $formatter; // Object used to format the data
  /**
   * Class for doing syntax replacements; 
   * @param $regexp
   * @return unknown_type
   */
  public function __construct($regexp, $trim, $formatter=NULL) {
    $this->tpattern = $regexp; 
    $this->trim_chars = $trim; 
    if (is_object($formatter)) { 
      $this->formatter=$formatter; 
    }
  }
  
	  /**
	 * Get the value from the data.  
	 * This is used by token_replace method to extract the data based on the path provided. 
	 * @param $data
	 * @param $key
	 * @return unknown_type
	 */
	protected function get_value($data, $key, $raw=FALSE)
	{
	  $retvar = ''; 
	  if (is_array($data))
	  {
	    $retvar = $data[$key];
	  } elseif (is_object($data)) {
	    $rows = $data->xpath($key);
	    if ($rows) $x = $rows[0];
	    if ($x) $retvar = $x->asXML(); 
	    if ($retvar) {
	      // Find the end of the first tag.
	      //print $root_tag; 
	      $p = strpos($retvar,'>'); 
	      $retvar = substr_replace($retvar,' ',0,$p+1);
	      $p = strrpos($retvar,'<',-1); 
	      $retvar = substr_replace($retvar,'',$p,strlen($retvar) - $p); 
	    }
	  }
	  // Call the formatter object if neccessary
	  $f = $this->formatter; 
	  if (!$raw && is_object($f) && method_exists($f,'format')) $retvar = $f->format($retvar,$key,$data);
	    
	  $retvar = trim($retvar); 
	  return $retvar; 
	}
  
  
  /**
   * 
   * @param $text text that needs replacing
   * @param $data
   * @return unknown_type
   */
  public function replace($text, $data) {
  	  $match=array();
      $o_text = $text;  
      if (preg_match_all($this->tpattern,$o_text,$match))
      {
	    //list($params) = $match[1];
	    $i=0; 

	    foreach($match[0] as $match_num=>$token)
	    {
	       $path = trim($token,$this->trim_chars);
	       $value = $this->get_value($data, $path); 
	       $text = str_replace($token, $value, $text);
	    }
    
      }
	  return $text;
  	
  }
}
