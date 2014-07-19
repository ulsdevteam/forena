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
  public $templateName = 'Graph (svg)';
  public $xy_data = FALSE;
  private $point_attributes;
  private $headers = array();
  private $weight;
  private $graphData;
  private $graphOptions;
  private $colors;
  // These field keys can be interpreted at the data point level, so we are going to add them.
  private $field_keys = array(
      'link',
      'tooltip',
      'colour',
      'label',
      'stroke_colour',
      'stroke_dash',
      'marker_type',
      'marker_size',
      'marker_stroke_width',
      'marker_stroke_colour'
  );

  // Place to indicate which fields are sourced from the data.
  private $field_sources = array();

  public function __construct() {
    parent::__construct();
    $library = forena_library_file('SVGGraph');
    require_once $library;
  }

  /**
   * Re-architect the data into something that the graphing engine can work with
   */
  public function generateGraphData(&$data, $series, $label) {

    // Default controling attributes
    $dim_headers = array($label);

    $this->graphOptions['structure']['value'] = array();
    foreach($series as $dk) {
      foreach($series as $col) {
        $this->graphOptions['structure']['value'][] = trim("$col", '{}');
      }
    }

    $this->graphData = array();
    foreach ($data as $row) {
      Frx::Data()->push($row, '_row');
      $trow = array();
      // Base group
      $trow['label'] =  $this->teng->replace($label);;
      // Dimensions
      foreach($series as $col) {
          $val = $this->teng->replace($col, TRUE);
          if(!$val) $val = 0;
          $trow[trim("$col", '{}')] = $val;
      }
      foreach($this->field_sources as $key => $src) {
        $trow[$key] = $this->teng->replace($src);
      }
      Frx::Data()->pop();
      $this->graphData[] = $trow;
    }
  }


   /**
   * Re-architect the data into something that the graphing engine can work with
   */
  public function generateGroupGraphData(&$block_data, $group, $series, $label, $dim) {
    $dim_headers = array();
    $dim_rows = array();
    $dim_columns = array();
    $group_headers = array();
    $dim_values = array();
    $rows = array();
    $data = $this->frxReport->group($block_data, $group);

    foreach ($data as $gk => $group_rows) {
      $row_copy = array_values($group_rows);
      $dims = $this->frxReport->group($group_rows, $dim);
      $rows[$gk] = $group_rows[0];
      foreach($dims as $dk=>$r) {
        $dims = array_values($r);
        $dim_values[$dk] = $dk;
        $dim_rows[$gk][$dk] = $r[0];
      }
    }

    // Default controling attributes
    $dim_headers = array($label);
    $dim_columns = $series;

    // Add the dimension headers.
    foreach ($dim_values as $dk) {
      foreach ($dim_headers as $i => $col) {
        $cell = '';
        if (count($dim_columns) > 1) {
          $cell = $i ? $col : $dk . ' ' . $col;
        }
        $hrow []  = $cell;
      }
    }

    $this->graphOptions['structure']['value'] = array();
    foreach($dim_values as $dk) {
      foreach($dim_columns as $col) {
        $this->graphOptions['structure']['value'][] = $dk . ' ' . trim("$col", '{}');
      }
    }

    $this->graphData = array();
    $label = '';
    foreach ($rows as $k=>$row) {
      Frx::Data()->push($row, '_group');
      $trow = array();
      // Base group

      $label = $this->teng->replace($group);
      $trow['label'] = $label;
      foreach($this->field_sources as $key => $src) {
        $trow[$key] = $this->teng->replace($src);
      }
      Frx::Data()->pop();
      // Dimensions
      $dim_data = $dim_rows[$k];
      foreach($dim_values as $dk) {
        $dim_row = isset($dim_data[$dk]) ? $dim_data[$dk] : array();
        frx::Data()->push($dim_row, '_dim');
        foreach($dim_columns as $col) {
          $cell = $col;
          $val = $this->teng->replace($col, TRUE);
          if(!$val) $val = 0;
          $trow[$dk . ' ' . trim("$col", '{}')] = $val;
        }
        frx::Data()->pop();
      }
      $this->graphData[] = $trow;

    }

    return $this->graphData;
  }

  /**
   * Add column for cross tabs.
   */
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

  /**
   * Derive config variables from graph.
   */
  public function scrapeConfig() {
    $this->weight = 0;
    $attrs = array();
    $svg = '';
    $nodes =  $this->reportDocNode->xpath('//svg');
    if ($nodes) {
      $svg = $nodes[0];
      $config = $this->mergedAttributes($svg);
    }

    // Determine graph type
    $graph_type =  isset($config['type']) ? $config['type'] : 'BarGraph';
    $types = $this->graphTypes();
    $types = array_change_key_case($types);
    $config['base_type'] = $types[$graph_type]['type'];

    // Get child table attributes for xml configuration.
    $nodes =  $this->reportDocNode->xpath('//table');
    if ($nodes) {
      $table = $nodes[0];
      $attrs = $this->mergedAttributes($table);
    }
    $config['group'] = $group = $attrs['group'];
    $config['dim']  = $dim = $attrs['dim'];
    $this->extractTemplateHTML($this->reportDocDomNode, $config, array('svg'));
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

  /**
   * Render the graph.
   * @return Ambigous <string, mixed>
   */
  public function render() {
    // Get data from source
    $output = '';
    $options = array();
    $series = array();
    $label = '';
    $links = '';
    $dim='';
    $seriesx = array();
    $attributes = $this->mergedAttributes();
    // Default in xpath for backward compatibility
    $path = isset($attributes['xpath']) ? $attributes['xpath'] :  '*';
    $path = isset($attributes['path']) ? $attributes['path'] :  $path;
    $group = @$attributes['group'];

    // Determine basic data to iterate.
    $data = Frx::Data()->currentContext();
    if (is_object($data)) {
      if (method_exists($data, 'xpath')) {
        $nodes = $data->xpath($path);
      }
      else {
        $nodes = $data;
      }
    }
    else {
      $nodes = (array)$data;
    }

    $type = $attributes['type'];
    $link = @$attributes['link'];


    // Legacy options.  New charts should be generated using Frx:attribute syntax
    if (isset($attributes['options'])) {
      parse_str($attributes['options'], $options);
      unset($attributes['options']);
      foreach ($options as $key => $value) {
        $options[$key] = $this->teng->replace($value, TRUE);
      }
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
    if (isset($options['dim'])) {
      $dim = $options['dim'];
      unset($options['dim']);
    }

    // Force structured data
    $options['structured_data'] = TRUE;
    $options['structure'] = array('key' => 'label');
    // ALlow overriding of chart data
    $this->field_sources = array();
    if (isset($options['color'])) $options['colour'] = $options['color'];
    $this->field_sources = array();
    foreach ($this->field_keys as $fk) {
      if (@$options[$fk] && strpos($options[$fk], '{')!==FALSE) {
        $this->field_sources[$fk] = $options[$fk];
        $options['structure'][$fk] = $fk;
      }
    }

    if (isset($attributes['height'])) $options['height'] = $this->teng->replace($attributes['height']);
    if (isset($attributes['width'])) $options['width'] = $this->teng->replace($attributes['width']);

    $this->graphOptions = $options;

    $limit = 3;
    if ($group && $dim) {
      $this->generateGroupGraphData($nodes, $group, $series, $label, $dim);
    }
    else {
      $this->generateGraphData($nodes, $series, $label);
    }

    // Deal with rare case where legend are supposed to come from data
    if (@$attributes['legend_entry'] && !is_array($attributes['legend_entry']) && strpos($attributes['legend_entry'], '{') !==FALSE) {
      $options['legend_entries'] = $this->xmlToValues($path, $attributes['legend_entry']);
    }



    if ($data) {
      $output = $this->renderChart($type, $data, $options, $links);
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

  function renderChart($type, $data ) {
    $options['width'] = @$options['width'] ? $options['width'] : 600;
    $options['height'] = @$options['height'] ? $options['height'] : 400;
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
      $output = $this->renderGraph($class);
    }
    return $output;
  }


  function renderGraph($type) {
    static $jsinc = FALSE;
    $options = $this->graphOptions;
    $data = $this->graphData;

    if (!isset($options['scatter_2d']) && ($type == 'ScatterGraph' || $type=='MultiScatterGraph') && $this->xy_data && !isset($options['scatter_2d'])) {
      $options['scatter_2d'] = TRUE;
    }
    else {
      $options['scatter_2d'] = (bool) @$options['scatter_2d'];
    }
    $width = (@$options['width']) ? @$options['width'] : 600;
    $height = (@$options['height']) ? @$options['height'] : 400;
    $graph = new SVGGraph($width, $height, $options);
    $this->graph = $graph;
    $graph->Values($data);
    if (isset($options['colour']) && is_array($options['colour'])) {
      $graph->Colours($options['colour']);
    }
    // Generate the graph
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
    $graph_options = FrxSVGGraph::graphOptions();
    $graph_types = FrxSVGGraph::graphTypes();
    $gt = array_change_key_case($graph_types);

    $type = @$config['type'];
    if (!isset($config['base_type'])) {
      $base_type = $type ? $gt[strtolower($type)]['type'] : 'Bar Graph';
    }
    else {
      $base_type = $config['base_type'];
    }
    $styles = $graph_options['styles'][$base_type];
    $types = $graph_options['types'];
    $form = parent::configForm($config);

    $form['base_type'] = array(
        '#type' => 'select',
        '#title' => t('Graph Type'),
        '#options' => $graph_options['types'],
        '#default_value' => $base_type,
        '#ajax' => $this->configAjax(),
    );

    $form['style'] = array(
        '#type' => 'select',
        '#title' => t('Style'),
        '#options' => $styles,
        '#default_value' => $type,
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

  public function configValidate(&$config) {
    $type = $config['style'];
    $base_type = $config['base_type'];
    $graph_options = $this->graphOptions();
    if(!array_key_exists($type, $graph_options['styles'][$base_type])) {
      $styles = array_keys($graph_options['styles'][$base_type]);
      $config['type'] = $config['style'] = array_shift($styles);
    }
    $config['type'] = $config['style'];
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
      if ($col['type'] == 'heading') $group[] = $col['contents'];
      if ($col['type'] == 'crosstab') $dim = $col['contents'];
    }
    if ($group) $config['group'] = is_array($group) ?  implode(' ', $group) : $group;
    if ($dim) $config['dim'] = $dim;
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
      $graph_options = FrxSVGGraph::graphOptions();

      // LImit the config
      $frxattrs = $this->arrayAttributes(array_intersect_key($config, array_fill_keys(array('path', 'series', 'series_x', 'label', 'group', 'link', 'dim'), 1 )));
      $frxattrs = $this->arrayAttributes($frxattrs);
      $frxattrs['renderer'] = 'FrxSVGGraph';
      $frxattrs['type'] = $type;


      $svg = $this->setFirstNode($div, 2, 'svg',  NULL, NULL,  $frxattrs);

    }

    $table_frx['renderer'] = 'FrxCrosstab';
    $table_frx['group'] = is_array($group) ? implode(' ', $group) : $group;
    $table_frx['dim'] = $dim;
    if (!$gen_table) $table_frx['hidden'] = '1';
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