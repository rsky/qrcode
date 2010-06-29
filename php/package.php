#!/usr/bin/env php
<?php
namespace MkPackage;

require_once 'PEAR/PackageFileManager2.php';
require_once 'SymfonyComponents/YAML/sfYaml.php';

\PEAR::setErrorHandling(PEAR_ERROR_DIE);
date_default_timezone_set('UTC');

define('PACAGE_XML', __DIR__ . '/package.xml');
define('PACAGE_YML', __DIR__ . '/package.yml');

if ($argc > 1 && $argv[1] === 'make') {
    $debug = false;
} else {
    $debug = true;
}
main($debug);

// {{{ main()

/**
 * Create package.xml
 *
 * @param   boolean $debug
 * @return  void
 */
function main($debug = true)
{
    if (file_exists(PACAGE_XML)) {
        fputs(STDOUT, 'ignore existing package.xml? [y/N]: ');
        $noChangeLog = (0 === strncasecmp(trim(fgets(STDIN)), 'y', 1));
    } else {
        $noChangeLog = false;
    }
    if ($noChangeLog) {
        rename(PACAGE_XML, PACAGE_XML . '.orig');
    }

    $package = init(configure(PACAGE_YML));
    $package->generateContents();

    if ($debug) {
        $package->debugPackageFile();
        if ($noChangeLog) {
            rename(PACAGE_XML . '.orig', PACAGE_XML);
        }
    } else {
        $package->writePackageFile();
    }
}

// }}}
// {{{ configure()

/**
 * Get the package configuration
 *
 * @param   string $yaml    Pathname of the YAML configuration file
 * @return  array
 */
function configure($yaml)
{
    $cfg = \sfYaml::load($yaml);
    $name = $cfg['name'];
    $options = $cfg['options'];

    if (!isset($options['filelistgenerator'])) {
        $cfg['options']['filelistgenerator'] = 'file';
    }

    if (!isset($options['baseinstalldir'])) {
        $cfg['options']['baseinstalldir'] = '/';
    }

    if (!isset($options['packagedirectory'])) {
        $cfg['options']['packagedirectory'] = __DIR__;
    }

    // ignored files
    $ignore = array(
        'package.php',
        'package.xml',
        'package.xml.orig',
        'package.yml',
        'config.w32',
        "{$name}.dsp",
        "{$name}-*.tgz",
    );
    if (isset($options['ignore'])) {
        $ignore = array_merge($ignore, $options['ignore']);
    }
    $cfg['options']['ignore'] = glob_values($ignore);

    // directory roles
    $dir_roles = array(
        'examples'  => 'data',
        'manual'    => 'doc',
        'tests'     => 'test',
    );
    if (isset($options['dir_roles'])) {
        $dir_roles = array_merge($dir_roles, $options['dir_roles']);
    }
    $cfg['options']['dir_roles'] = $dir_roles;

    // role exceptions
    $exceptions = array(
        'CREDITS'       => 'doc',
        'EXPERIMENTAL'  => 'doc',
        'LICENSE'       => 'doc',
        'README'        => 'doc',
    );
    if (isset($options['exceptions'])) {
        $exceptions = array_merge($exceptions, $options['exceptions']);
    }
    $cfg['options']['exceptions'] = glob_keys($exceptions);

    return $cfg;
}

// }}}
// {{{ glob_values()

/**
 * Extract wildcard for each values
 *
 * @param   array $list
 * @return  array
 */
function glob_values(array $list)
{
    $extended = array();

    foreach ($list as $filename) {
        if (strpos($filename, '*') === false) {
            $extended[] = $filename;
        } else {
            foreach (\glob($filename) as $tmp) {
                $extended[] = $tmp;
            }
        }
    }

    return $extended;
}

// }}}
// {{{ glob_keys()

/**
 * Extract wildcard for each keys
 *
 * @param   array $list
 * @return  array
 */
function glob_keys(array $list)
{
    $extended = array();

    foreach ($list as $filename => $value) {
        if (strpos($filename, '*') === false) {
            $extended[$filename] = $value;
        } else {
            foreach (\glob($filename) as $tmp) {
                $extended[$tmp] = $value;
            }
        }
    }

    return $extended;
}

// }}}
// {{{ init()

/**
 * Initialize PackageFileManager
 *
 * @param   array $cfg
 * @return  PEAR_PackageFileManager2
 */
function init(array $cfg)
{
    extract($cfg);

    $package = new \PEAR_PackageFileManager2;
    $package->setOptions($options);

    $package->setPackage($name);
    $package->setSummary($summary);
    $package->setNotes($notes);
    $package->setDescription($description);
    $package->setLicense($license, $licenseUri);

    $package->setReleaseVersion($version);
    $package->setAPIVersion($apiVersion);
    $package->setReleaseStability($stability);
    $package->setAPIStability($apiStability);

    foreach ($maintainers as $maintainer) {
        $package->addMaintainer($maintainer['role'],
                                $maintainer['handle'],
                                $maintainer['name'],
                                $maintainer['email'],
                                $maintainer['active'] ?: 'yes');
    }

    $package->setPackageType($type);

    if (strpos($type, 'ext') !== false) {
        $package->setProvidesExtension($name);

        if (strpos($type, 'extsrc') !== false && isset($configureOptions)) {
            foreach ($configureOptions as $configureOption) {
                $package->addConfigureOption($configureOption['name'],
                                             $configureOption['prompt'],
                                             $configureOption['default']);
            }
        }
    }

    $package->setPhpDep($phpDep);
    $package->setPearinstallerDep($pearDep);

    if (isset($packageDeps)) {
        foreach ($packageDeps as $packageDeps) {
            $min = $packageDeps['min'] ?: false;
            $max = $packageDeps['max'] ?: false;
            $recommended = $packageDeps['recommended'] ?: false;
            $exclude = $packageDeps['exclude'] ?: false;
            $nodefault = $packageDeps['nodefault'] ?: false;

            $package->addSubpackageDepWithChannel($packageDeps['type'],
                                                  $packageDeps['name'],
                                                  $packageDeps['channel'],
                                                  $min, $max, $recommended,
                                                  $exclude, $nodefault);
        }
    }

    $package->setChannel($channel);

    return $package;
}

// }}}

/*
 * Local Variables:
 * mode: php
 * coding: utf-8
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
// vim: set syn=php fenc=utf-8 ai et ts=4 sw=4 sts=4 fdm=marker:
