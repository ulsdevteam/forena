<?php
// $Id$
/**
 * @file
 * FrXSytnaxEngine defines how regular expression procesing/token substitution takes place.
 * It includes support for passing in a formatter oobject that will escape strings properly
 * before substituting them.
 *
 */
define('FRX_TOKEN_EXP', '/\{[^\n^\r^}]+}/');
define('FRX_SQL_TOKEN', '/:([a-z]|[0-9]|{A-Z]|[_])+/');
class FrxSyntaxEngine {
  private $tpattern;
  private $trim_chars;
  private $formatter; // Object used to format the data
  private $data_stack;
  private $data_sources;
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
    $this->data_stack = array();
    $this->data_sources = array();
  }

  /**
   * Push a data context onto the data stacks
   * to make sure that we can address these using an
   * appropriate syntax.  I think we don't need data_stack
   * but i'm holding it there in case we develop a "relative" path syntax.
   * @param $data
   * @param $id
   * @return unknown_type
   */
  public function push_data($data, $id='') {
    $this->data_stack[] = $data;

    if ($id) {
      $this->data_sources[$id] = $data;
    }
  }

  /**
   * Remove data from the data stack.  This will make data unavaiable
   * when we leave the context of the current nested reports.
   * @param $id
   * @return unknown_type
   */
  public function pop_data($id='') {
    array_pop($this->data_stack);
    if ($id) {
      unset($this->data_stack[$id]);
    }
  }

  /**
   * Provides an api to the {=xpath} syntax that can be used
   * to evaluat expressions such as sum and count in a report.  We
   * need to use the DOM object here, because this method isn't exposed
   * with simplexml.
   *
   * @param $xml
   * @param $path
   * @return unknown_type
   */
  protected function simplexml_evaluate($xml, $path) {
    $dom_node = dom_import_simplexml($xml);
    $dom_doc = new DOMDocument('');
    $dom_node = $dom_doc->importNode($dom_node, TRUE);
    $dom_doc->appendChild($dom_node);
    // Do we also need to call AppendChild?
    $xpath = new DOMXpath($dom_doc);
    $ret = $xpath->evaluate($path, $dom_node);
    return $ret;
  }


  /**
  * Get the value from the data.
  * This is used by token_replace method to extract the data based on the path provided.
  * @param $data
  * @param $key
  * @return unknown_type
  */
  protected function get_value( $key, $raw=FALSE) {
    $retvar = '';
    // Determne which $data var we're going to get
    $data = array();
    if ($this->data_stack) {
      $i = count($this->data_stack) - 1;
      $data = $this->data_stack[$i];
    }
    $raw_key=$key;
    // Determine if we have a . syntax for the id.
    if ($key) {
      @list($id, $path) = explode('.', $key, 2);
      if ($data && isset($this->data_sources[$id])) {
        $data = $this->data_sources[$id];
        $key = $path;
      }
    }


    if (is_array($data)) {
      $retvar = @$data[$key];
    }
    elseif (is_object($data)) {

      if (strpos($key, '=')===0) {
        $retvar = $this->simplexml_evaluate($data, ltrim($key, '='));
      }
      else {
        $rows = $data->xpath($key);
        $x ='';
        if ($rows) $x = $rows[0];

        if ($x) $retvar = $x->asXML();
        // Check to see if there are child nodes
        // If so use asXML otherwise string cast.
        if ($retvar && strpos($retvar, '<')!==FALSE) {
          // Find the end of the first tag.
          $p = strpos($retvar, '>');
          $retvar = substr_replace($retvar, '', 0, $p+1);
          $p = strrpos($retvar, '<', -1);
          $retvar = substr_replace($retvar, '', $p, strlen($retvar) - $p);

        }
        else {
          $retvar = (string)$x;
        }
      }

    }
    // Call the formatter object if neccessary
    $f = $this->formatter;
    if (!$raw && is_object($f) && method_exists($f, 'format')) {
      $retvar = $f->format($retvar, $raw_key, $data);
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
  public function replace($text, $data='', $raw=FALSE) {
    $match=array();
    $o_text = $text;
    // Put the data on the stack.
    if ($data) $this->push_data($data);
    if (preg_match_all($this->tpattern, $o_text, $match)) {
      //list($params) = $match[1];
      $i=0;

      foreach ($match[0] as $match_num => $token) {
         $path = trim($token, $this->trim_chars);
         $value = $this->get_value($path, $raw);

          $pos = strpos($text, $token);
          if ($pos !== FALSE) {
            $text = substr_replace($text, $value, $pos, strlen($token));
          }
      }
    }
    if ($data) $this->pop_data();
    return $text;
  }

  /**
   * List all of the tokens used in a piece of text, ignoring duplicates.
   *
   * @param string $text
   * @return array tokens contained in the text according to the regular expression.
   */
  public function tokens($text) {
   $match=array();
   $tokens = array();

    if (preg_match_all($this->tpattern, $text, $match)) {
      $i=0;
      foreach ($match[0] as $match_num => $token) {
         $path = trim($token, $this->trim_chars);
         if (array_search($path, $tokens)===FALSE) {
           $tokens[] = $path;
         }
      }
    }
    return $tokens;
  }
}
