#include "chsettingsdialog.h"
#include "ui_chsettingsdialog.h"
#include <QSettings>
#ifdef _OPENMP
#include <omp.h>
#endif

CHSettingsDialog::CHSettingsDialog(QWidget *parent) :
	 QWidget(parent),
	 m_ui(new Ui::CHSettingsDialog)
{
	 m_ui->setupUi(this);
}

CHSettingsDialog::~CHSettingsDialog()
{
	 delete m_ui;
}

bool CHSettingsDialog::getSettings( Settings* settings )
{
	if ( settings == NULL )
		return false;
	settings->blockSize = m_ui->blockSize->value();
	return true;
}

bool CHSettingsDialog::loadSettings( QSettings* settings )
{
	settings->beginGroup( "Contraction Hierarchies" );
	m_ui->blockSize->setValue( settings->value( "blockSize", 12 ).toInt() );
	settings->endGroup();
	return true;
}

bool CHSettingsDialog::saveSettings( QSettings* settings )
{
	settings->beginGroup( "Contraction Hierarchies" );
	settings->setValue( "blockSize", m_ui->blockSize->value() );
	settings->endGroup();
	return true;
}
