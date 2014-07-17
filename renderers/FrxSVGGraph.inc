<?php
/**
 * @file FrxSVGGraph
 * php SVG Graph generator
 *
 * @author davidmetzler
 *
 */
class FrxSVGGraph extends FrxRenderer {
  private $graph;
  private $links;
  public $templateName = 'SVG Graph';
  public $xy_data = FALSE;
  private $headers = array();
  private $dim_columns = array();
  private $group_columns = array();
  private $dim_headers = array();
  private $group_headers = array();
  private $weight;

  public function __construct() {
    parent::__construct();
    $library = forena_library_file('SVGGraph');
    require_once $library;
  }
  private function addColumn($type, $token, $label,  &$config) {
    $key = trim($token, '{}');
    $this->weight++;
    $config['crosstab_columns'][$key] = array(
        'contents' => $token,
        'label' => $label,
        'type' => $type,
        'weight' => $this->weight,
    );
  }
  public function scrapeConfig() {
    $this->weight = 0;
    $attrs = array();
    $svg = '';
    $nodes =  $this->reportDocNode->xpath('//svg');
    if ($nodes) {
      $svg = $nodes[0];
      $config = $this->mergedAttributes($svg);
    }
    $graph_type =  isset($config['type']) ? strtolower($config['type']) : 'bargraph';
    $types = $this->graphTypes();
    $types = array_change_key_case($types);
    $config['type'] = $types[$graph_type]['type'];
    $config['style'] = $types[$graph_type]['style'];

    // Get child table attributes for xml configuration.
    $nodes =  $this->reportDocNode->xpath('//table');
    if ($nodes) {
      $table = $nodes[0];
      $attrs = $this->mergedAttributes($table);
    }
    $config['group'] = $group = $attrs['group'];
    $config['dim']  = $dim = $attrs['dim'];
    $this->extractTemplateHTML($this->reportDocDomNode, $config, array('table'));
    $head_ths = $this->extractXPathInnerHTML('*//thead/tr/th', $this->reportDocDomNode, FALSE);
    $head_tds = $this->extractXPathInnerHTML('*//thead/tr/td', $this->reportDocDomNode, FALSE);
    $body_ths = $this->extractXPathInnerHTML('*//tbody/tr/th', $this->reportDocDomNode, FALSE);
    $body_tds = $this->extractXPathInnerHTML('*//tbody/tr/td', $this->reportDocDomNode, FALSE);
    $heading_cols = array_combine($head_ths, $body_ths);
    $data_cols = array_combine($head_tds, $body_tds);
    // Get the named headers
    foreach($heading_cols as $label=>$token) {
      $this->addColumn('heading', $token, $label, $config);
    }
    // Get the data cells
    if ($dim) {
      $dims = (array)$dim;
      foreach($dims as $dim) {
        $this->addColumn('crosstab', $dim, trim($dim, '{}'), $config);
      }
    }
    foreach($data_cols as $label=>$token) {
      $this->addColumn('value', $token, $label, $config);
    }

    return $config;
  }

  public function render() {

    // Get data from source
    $output = '';
    $options = array();
    $series = array();
    $seriesx = array();
    $data = array();
    $attributes = $this->mergedAttributes();
    $path = isset($attributes['xpath']) ? $attributes['xpath'] :  '*';
    $label = '';
    $links = '';
    $type = $attributes['type'];
    $link = @$attributes['link'];
    // Legacy options.  New charts should be generated using Frx:attribute syntax
    if (isset($attributes['options'])) {
      parse_str($attributes['options'], $options);
      unset($attributes['options']);
      $options  = array_merge($attributes, $options);
    }
    else {
      $options = $attributes;
    }

    $series = @$options['series'];
    if (isset($options['seriesx']))  {
      $seriesx = $options['seriesx'];
      unset($options['seriesx']);
    }
    if (isset($options['label'])) {
      $label =  $options['label'];
      unset($options['label']);
    }
    foreach ($options as $key => $value) {
      $options[$key] = $this->teng->replace($value, TRUE);
    }

    if (isset($attributes['height'])) $options['height'] = $this->teng->replace($attributes['height']);
    if (isset($attributes['width'])) $options['width'] = $this->teng->replace($attributes['width']);

    // Extract chart series data from xml.
    if ($series) {
      foreach ((array)$series as $i => $d) {
        if ($seriesx) {
          $this->xy_data = TRUE;
          $data[] = $this->xmlToValues($path, $d, $seriesx[$i], TRUE);
        }
        else {
          $data[] = $this->xmlToValues($path, $d, $label, FALSE);
        }
      }
    }

    if ($link) {
      $links = $this->xmlToValues($path, $link, $label);
    }

    // Deal with rare case where attributes are supposed to come from data
    if (@$attributes['color'] && !is_array($attributes['color']) && strpos($attributes['color'], '{') !==FALSE) {
      $options['colors'] = $this->xmlToValues($path, $attributes['color']);
    }

    // Deal with rare case where legend are supposed to come from data
    if (@$attributes['legend_entry'] && !is_array($attributes['legend_entry']) && strpos($attributes['legend_entry'], '{') !==FALSE) {
      $options['legend_entries'] = $this->xmlToValues($path, $attributes['legend_entry']);
    }


    if ($data) {

      if (method_exists($this, 'renderChart')) {
        $output = $this->renderChart($type, $data, $options, $links);
      }

    }
    return $output;
  }

  static function graphTypes() {
    return array(
      'BarGraph' => array('type' => 'Bar Graph', 'style' => 'Simple', 'series' => 1),
      'Bar3DGraph' => array('type' => 'Bar Graph', 'style' => '3D', 'series' => 1),
      'StackedBarGraph' => array('type' => 'Bar Graph', 'style' => 'Stacked', 'series' => 4),
      'GroupedBarGraph' => array('type' => 'Bar Graph', 'style' => 'Grouped', 'series' => 4),
      'CylinderGraph' => array('type' => 'Bar Graph', 'style' => 'Cylinder', 'series' => 1),
      'StackedCylinderGraph' => array('type' => 'Bar Graph', 'style' => 'Stacked Cylinder', 'series' => 4),
      'GroupedCylinderGraph' =>  array('type' => 'Bar Graph', 'style' => 'Grouped Cylinder', 'series' => 1),
      'PieGraph' => array('type' => 'Pie Chart', 'style' => 'Simple', 'series' => 1),
      'Pie3DGraph' => array('type' => 'Pie Chart', 'style' => '3D', 'series' => 1),
      'HorizontalBarGraph' => array('type' => 'Bar Graph', 'style' => 'Horizontal', 'series' => 1),
      'LineGraph' =>array('type' => 'Line Graph', 'style' => 'Simple', 'series' => 1),
      'MultiLineGraph' => array('type' => 'Line Graph', 'style' => 'Multi', 'series' => 4),
      'ScatterGraph' => array('type' => 'Scatter Plot', 'style' => 'Simple', 'series' => 1, 'xaxis' => TRUE),
      'MultiScatterGraph' => array('type' => 'Scatter Plot', 'style' => '3D', 'series' => 4, 'xaxis' => TRUE),
      'RadarGraph' => array('type' => 'Radar Graph', 'style' => 'Simple', 'series' => 1),
      'MultiRadarGraph' => array('type' => 'Radar Graph', 'style' => 'Multi', 'series' => 4),
    );
  }

  static function graphOptions() {
    $data = FrxSVGGraph::graphTypes();
    foreach($data as $key => $value) {
      $type[$value['type']] = $value['type'];
      $style[$value['type']][$key] = $value['style'];
    }
    return array('types' => $type, 'styles' => $style);
  }

  function renderChart($type, $data, $options, $links ) {
    $options['width'] = @$options['width'] ? $options['width'] : 600;
    $options['height'] = @$options['height'] ? $options['height'] : 400;
    $this->links = $links;
    $type = strtolower($type);
    // Legacy sustitions for backcward compatibility.
    if ($type == 'piechart') $type = 'piegraph';
    if ($type == 'scatterplot') $type = 'scattergraph';
    if ($type == 'multiscatterplot') $type = 'multiscattergraph';

    // Newly defined types
    $graph_types = FrxSVGGraph::graphTypes();

    // Build map for array types.
    $lower_graphs_types = array_change_key_case($graph_types);
    $graph_classes = array_combine(array_keys($lower_graphs_types), array_keys($graph_types));


    if ($data && isset($graph_classes[$type])) {
      $class = $graph_classes[$type];
      if (@$graph_types[$class]['series'] == 1) {
        $output = $this->renderGraph($class, $options, $data[0]);
      }
      else {
        $output = $this->renderGraph($class, $options, $data);
      }
    }
    return $output;
  }


  function renderGraph($type, $options, $data) {
    static $jsinc = FALSE;
    if (!isset($options['scatter_2d']) && ($type == 'ScatterGraph' || $type=='MultiScatterGraph') && $this->xy_data && !isset($options['scatter_2d'])) {
      $options['scatter_2d'] = TRUE;
    }
    else {
      $options['scatter_2d'] = (bool) @$options['scatter_2d'];
    }
    $links = $this->links;
    $width = (@$options['width']) ? @$options['width'] : 600;
    $height = (@$options['height']) ? @$options['height'] : 400;
    $graph = new SVGGraph($width, $height, $options);
    $this->graph = $graph;
    $graph->Values($data);
    if (is_array(@$options['colors'])) {
      $graph->colours = $options['colors'];
    }

    if ($links) {
      $graph->Links($links);
    }
    $output = $graph->Fetch($type, FALSE);
    // Add a viewbox to be compatible with Prince PDF generation.
    $output = str_replace('<svg width', "<svg viewBox='0 0 $width $height' width", $output);
    $javascript = '/<script[^>]*?>.*?<\/script>/si';
    $noscript = '';
    if (!$jsinc && $this->format == 'web') {
      $output .= $graph->FetchJavascript();
    }
    else {
      $output = preg_replace($javascript, $noscript, $output);
    }

    return $output;
  }

  /**
   * Generate series from crosstab.
   * @param unknown $config
   */
  private function seriesFromColumns(&$config) {
    $series = array();
    foreach ($config['crosstab_columns'] as $key => $col) {
      if ($col['type'] == 'value') {
        $series[] = '{' . $key . '}';
      }
    }
    $config['series'] = $series;
  }

  private function labelFromColumns(&$config) {
    $label = '';
    foreach ($config['crosstab_columns'] as $key => $col) {
      if ($col['type'] == 'heading') {
        $label = trim($label . ' {' . $key . '}');
      }
    }
    $config['label'] = $label;
  }

  public function configForm($config) {
    $form = array();

    $graph_types = array(
        'bargraph' => 'Bar Graph',
        'linegraph' => 'Line Graph',
        'multilinegraph' => 'Line Graph (Multi Series)',
        'piechart' => 'Pie Chart',
        'radargraph' => 'Radar Graph',
        'multiradargraph' => 'Radar Graph (Multi Series)',
        'scatterplot' => 'Scatter Plot',
    );


    $type = isset($config['type']) ? $config['type'] : 'Bar Graph';
    $style = isset($config['style']) ? $config['style'] : 'BarGraph';
    $graph_options = FrxSVGGraph::graphOptions();
    $graph_types = FrxSVGGraph::graphTypes();
    $styles = $graph_options['styles'][$type];
    $xvalues = @$graph_types[$type]['xaxis'];
    $num_series = isset($graph_types[$style]['series']) ? $graph_types[$style]['series'] : 1;
    $types = $graph_options['types'];

    $form = parent::configForm($config);

    $form['xpath'] = array(
      '#type' => 'textfield',
      '#title' => ('xpath'),
      '#default_value' => @$config['xpath'],
    );

    $form['type'] = array(
        '#type' => 'select',
        '#title' => t('Graph Type'),
        '#options' => $graph_options['types'],
        '#default_value' => $type,
        '#ajax' => $this->configAjax(),
    );

    $form['style'] = array(
        '#type' => 'select',
        '#title' => t('Style'),
        '#options' => $styles,
        '#default_value' => $style,
        '#ajax' => $this->configAjax(),
    );

    $form['gen_table'] = array(
        '#type' => 'checkbox',
        '#title' => t('Include data table with graph'),
        '#default_value' => @$config['gen_table'],
        '#ajax' => $this->configAjax(),
    );


    $this->weight_sort($config['crosstab_columns']);
    $types = array('heading' => t('Label'), 'crosstab' => t('Crosstab'), 'value' => 'Value', 'ignore' => t('Ignore'));
    $form['crosstab_columns'] = array('#theme' => 'forena_element_draggable',   '#draggable_id' => 'FrxCrosstab-columns');
    foreach ($config['crosstab_columns'] as $key => $col) {
      $ctl = array();
      $ctl['label'] = array(
          '#type' => 'textfield',
          '#size' => 30,
          '#title' => t('Label'),
          '#default_value' => $col['label'],
      );

      $ctl['contents'] = array(
          '#type' => 'textfield',
          '#size' => '30',
          '#title' => t('Data'),
          '#default_value' => $col['contents'],
      );

      $ctl['type'] = array(
          '#type' => 'radios',
          '#title' => t('Type'),
          '#default_value' => $col['type'],
          '#options' => $types,
          '#ajax' => $this->configAjax()
      );

      $ctl['weight'] = array(
          "#type" => 'weight',
          '#title' => t('Weight'),
          '#delta' => 50,
          '#default_value' => $col['weight'],
      );

      $form['crosstab_columns'][$key] = $ctl;
    }


    $form['link'] = array(
        '#type' => 'textfield',
        '#title' => 'link',
        '#default_value' => @$config['link'],
    );

    return $form;
  }

  public function generate($xml, &$config) {
    $config['class'] = get_class($this);
    $media = 'FrxSVGGraph';
    $div = $this->blockDiv($config);
    $attrs  = array();

    // Determine columns and make sure we represent them all
    $found_columns = $this->columns($xml);
    if (!$found_columns) {
      $found_columns = $this->columns($xml, '/*');
    }

    $numeric_columns = $this->numeric_columns;
    $new_columns = @$config['crosstab_columns'] ? FALSE : TRUE;
    foreach ($found_columns as $column => $label) {
      $token = '{' . $column . '}';
      if ($new_columns) {
        $type = isset($numeric_columns[$column]) ? 'value' : 'heading';
      } else {
        $type = 'ignore';
      }
      if (!isset($config['crosstab_columns'][$column])) {
        $this->addColumn($type, '{' . $column . '}', $column, $config);
      }
    }

    // Generate the grouping row
    $group = '';
    $dim = array();
    foreach($config['crosstab_columns'] as $col) {
      if ($col['type'] == 'heading') $group .= $col['contents'];
      if ($col['type'] == 'crosstab') $dim = $col['contents'];
    }

    // PUt on the header
    $this->removeChildren($div);
    if (isset($config['header']['value'])) $this->addFragment($div, $config['header']['value']);


    if ($config) {
      $this->seriesFromColumns($config);
      $this->labelFromColumns($config);


      // Clean colors
      if (isset($config['colors'])) foreach ($config['colors'] as $i => $color) if (!$color) unset($color[$i]);

      $type = $this->extract('type', $config);
      if (!$type) $type = 'Bar Graph';

      $gen_table = $this->extract('gen_table', $config);
      $style = $this->extract('style', $config);
      if (!$style) $style = 'BarGraph';
      $graph_options = FrxSVGGraph::graphOptions();
      if (!isset($graph_options['styles'][$type][$style])) {
        $styles = array_keys($graph_options['styles'][$type]);
        $style = $styles[0];
      }
      if ($style) $type = $style;

      // LImit the config
      $frxattrs = $this->arrayAttributes(array_intersect_key($config, array_fill_keys(array('path', 'series', 'series_x', 'label', 'group', 'link'), 1 )));
      $frxattrs = $this->arrayAttributes($frxattrs);
      $frxattrs['renderer'] = 'FrxSVGGraph';
      $frxattrs['type'] = $type;


      $svg = $this->setFirstNode($div, 2, 'svg',  NULL, NULL,  $frxattrs);

    }

    $table_frx['renderer'] = 'FrxCrosstab';
    $table_frx['group'] = $group;
    $table_frx['dim'] = $dim;
    $table = $this->setFirstNode($div, 4, 'table', NULL, $attrs, $table_frx);
    $thead = $this->setFirstNode($table, 6, 'thead');
    $throw = $this->setFirstNode($thead, 8, 'tr');
    $tbody = $this->setFirstNode($table, 6, 'tbody');
    $tdrow = $this->setFirstNode($tbody, 8, 'tr', NULL, NULL, $attrs);
    if ($config['crosstab_columns']) foreach ($config['crosstab_columns'] as $key => $col) if ($col['type']!=='ignore') {
      if ($col['type']=='heading') {
        $tag = 'th';
      }
      else {
        $tag = 'td';
      }
      if ($col['type'] != 'crosstab') {
        $this->addNode($throw, 10, $tag, $col['label']);
        $this->addNode($tdrow, 10, $tag, $col['contents']);
      }
    }
    if (isset($config['footer']['value'])) $this->addFragment($div, $config['footer']['value']);
  }



}