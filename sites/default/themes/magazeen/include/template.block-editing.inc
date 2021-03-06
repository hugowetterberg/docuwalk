<?php
// $Id: template.block-editing.inc,v 1.1.2.2 2009/05/04 17:10:38 zinv Exp $

/**
 * @file
 * Contains functions only needed if the user has block editing permissions.
 */

/**
 * Add block editing variables into the block templates.
 *
 * @param $vars
 *   An array of variables to pass to the theme template.
 * @param $hook
 *   The name of the template being rendered ("block" in this case.)
 */
function zen_preprocess_block_editing(&$vars, $hook) {
  $block = $vars['block'];

  // Display 'edit block' for custom blocks.
  if ($block->module == 'block') {
    $edit_links[] = l('<span>' . t('edit block') . '</span>', 'admin/build/block/configure/' . $block->module . '/' . $block->delta,
      array(
        'attributes' => array(
          'title' => t('edit the content of this block'),
          'class' => 'block-edit',
        ),
        'query' => drupal_get_destination(),
        'html' => TRUE,
      )
    );
  }
  // Display 'configure' for other blocks.
  else {
    $edit_links[] = l('<span>' . t('configure') . '</span>', 'admin/build/block/configure/' . $block->module . '/' . $block->delta,
      array(
        'attributes' => array(
          'title' => t('configure this block'),
          'class' => 'block-config',
        ),
        'query' => drupal_get_destination(),
        'html' => TRUE,
      )
    );
  }

  // Display 'edit view' for Views blocks.
  if ($block->module == 'views' && user_access('administer views')) {
    $edit_links[] = l('<span>' . t('edit view') . '</span>', 'admin/build/views/' . $block->delta . '/edit',
      array(
        'attributes' => array(
          'title' => t('edit the view that defines this block'),
          'class' => 'block-edit-view',
        ),
        'query' => drupal_get_destination(),
        'fragment' => 'edit-block',
        'html' => TRUE,
      )
    );
  }
  // Display 'edit menu' for Menu blocks.
  elseif (($block->module == 'menu' || ($block->module == 'user' && $block->delta == 1)) && user_access('administer menu')) {
    $menu_name = ($block->module == 'user') ? 'navigation' : $block->delta;
    $edit_links[] = l('<span>' . t('edit menu') . '</span>', 'admin/build/menu-customize/' . $menu_name,
      array(
        'attributes' => array(
          'title' => t('edit the menu that defines this block'),
          'class' => 'block-edit-menu',
        ),
        'query' => drupal_get_destination(),
        'html' => TRUE,
      )
    );
  }
  // Display 'edit menu' for Menu block blocks.
  elseif ($block->module == 'menu_block' && user_access('administer menu')) {
    $menu_name = variable_get('menu_block_' . $block->delta . '_menu_name', 'navigation');
    $edit_links[] = l('<span>' . t('edit menu') . '</span>', 'admin/build/menu-customize/' . $menu_name,
      array(
        'attributes' => array(
          'title' => t('edit the menu that defines this block'),
          'class' => 'block-edit-menu',
        ),
        'query' => drupal_get_destination(),
        'html' => TRUE,
      )
    );
  }

  $vars['edit_links_array'] = $edit_links;
  $vars['edit_links'] = '<div class="edit">' . implode(' ', $edit_links) . '</div>';
}
