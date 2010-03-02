<?php

class FrxSyntaxEngine { 
  private $tpattern; 
  
  /**
   * Class for doing syntax replacements; 
   * @param $regexp
   * @return unknown_type
   */
  public function __contsruct(string $regexp) {
    $this->tpattern = $regexp; 
  }
  
	  /**
	 * Get the value from the data.  
	 * This is used by token_replace method to extract the data based on the path provided. 
	 * @param $data
	 * @param $key
	 * @return unknown_type
	 */
	protected function get_value($data,$key)
	{
	  $retvar = ''; 
	  if (is_array($data))
	  {
	    $retvar = $data[$key];
	  } elseif (is_object($data)) {
	    $rows = $data->xpath($key);
	    if ($rows) $x = $rows[0];
	    $retvar = $x->asXML(); 
	    if ($retvar) {
	      // Find the end of the first tag.
	      //print $root_tag; 
	      $p = strpos($retvar,'>'); 
	      $retvar = substr_replace($retvar,' ',0,$p+1);
	      $p = strrpos($retvar,'<',-1); 
	      $retvar = substr_replace($retvar,'',$p,strlen($retvar) - $p); 
	    }
	  }
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
      if (preg_match_all('/\{[^}]+}/',$o_text,$match))
      {
	    //list($params) = $match[1];
	    $i=0; 
	    foreach($match[0] as $match_num=>$token)
	    {
	       $path = trim($token,'{}');
	       $value = $this->get_value($data, $path); 
	       $text = str_replace($token, $value, $text);
	    }
    
      }
	  return $text;
  	
  }
}
