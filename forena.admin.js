/**
 * @file
 * Legacy forena behaviors.  These are deprecated. 
 */
(function ($) {

  Drupal.behaviors.forenaAdmin = {
    attach: function (context, settings) {
      if ($.fn.dataTable) {
        $('table.dataTable-paged').dataTable({
          "sPaginationType": "full_numbers"
        });

        $('#forena-block-preview .FrxTable table').dataTable({
          "sPaginationType": "full_numbers",
          "scrollX": true
        });
      }
    }
  };

})(jQuery);
