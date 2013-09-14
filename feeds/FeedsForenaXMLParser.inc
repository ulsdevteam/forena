<?php
class FeedsForenaXMLParser extends FeedsParser {

  /**
   * Implements FeedsParser::parse().
   */
  public function parse(FeedsSource $source, FeedsFetcherResult $fetcher_result) {
    $source_config = $source->getConfigFor($this);
    $result = new FeedsParserResult();
    $fields = @$source_config['fields'];
    $parser = Frx::Fields($fields);
    // Load and configure parser.
    $xpath = @$source_config['xpath'] ? $source_config['xpath'] : '*';
    $raw = $fetcher_result->getRaw();
    $mappings = feeds_importer($this->id)->processor->config['mappings'];


    if ($raw) {
      $xml = new SimpleXMLElement($raw);
      $nodes = $xml->xpath($xpath);
      foreach($nodes as $node) {
        Frx::Data()->push($node, 'row');
        $item = array();
        foreach($mappings as $field) {
          $text = $field['source'];
          $item[$text] = $parser->render($text);
        }
        $result->items[] = $item;
        Frx::Data()->pop();
      }
    }
    // Create a result object and return it.
    return $result;
  }


  /**
   * Define defaults.
   */
  public function sourceDefaults() {
    return array(
      'xplath' => $this->config['xpath'],
    );
  }

  /**
   * Source form.
   *
   * Show mapping configuration as a guidance for import form users.
   */
  public function sourceForm($source_config) {
    $form = array();
    $form['#weight'] = -10;

    $mappings = feeds_importer($this->id)->processor->config['mappings'];
    $sources = $uniques = array();
    foreach ($mappings as $mapping) {
      $sources[] = check_plain($mapping['source']);
      if (!empty($mapping['unique'])) {
        $uniques[] = check_plain($mapping['source']);
      }
    }

    $items = array();
    $items[] = format_plural(count($uniques), t('Column <strong>!column</strong> is mandatory and considered unique: only one item per !column value will be created.', array('!column' => implode(', ', $uniques))), t('Columns <strong>!columns</strong> are mandatory and values in these columns are considered unique: only one entry per value in one of these column will be created.', array('!columns' => implode(', ', $uniques))));
    $form['help'] = array(
      'list' => array(
        '#theme' => 'item_list',
        '#items' => $items,
      ),
    );
    $form['xpath'] = array(
      '#type' => 'textfield',
      '#title' => t('XPath Expression'),
      '#description' => t('Specify an xpath expression that will be used to break up the data into feed items.  When in doubt use * '),
      '#default_value' => isset($source_config['xpath']) ? $source_config['xpath'] : '*',
    );
    return $form;
  }

  /**
   * Define default configuration.
   */
  public function configDefaults() {
    return array(
      'xpath' => '*',
    );
  }


  /**
   * Build configuration form.
   */
  public function configForm(&$form_state) {
    $form = array();
    $form['xpath'] = array(
      '#type' => 'textfield',
      '#title' => t('XPath Expression'),
      '#description' => t('Specify an xpath expression that will be used to break up the data into feed items.  When in doubt use * '),
      '#default_value' => $this->config['xpath'],
    );
    return $form;
  }

}
