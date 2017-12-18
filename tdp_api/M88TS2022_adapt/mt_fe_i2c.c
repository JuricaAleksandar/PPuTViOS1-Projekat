/****************************************************************************
* MONTAGE PROPRIETARY AND CONFIDENTIAL
* Montage Technology (Shanghai) Inc.
* All Rights Reserved
* --------------------------------------------------------------------------
*
* File:				mt_fe_i2c.c
*
* Current version:	1.00.00
*
* Description:		Define all i2c function for FE module.
*
* Log:	Description			Version		Date		Author
*		---------------------------------------------------------------------
*		Create				1.00.00		2010.09.15	YZ.Huang
*		Modify				1.00.00		2010.09.15	YZ.Huang
****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
//#include <ktlv.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mt_fe_def.h"
#include "i2c_adapt.h"
//#include "mt_fe_common_functions.h"

extern void _mt_sleep(U32 ms);

#define DEV_ADDR            0x68
#define DEV_ADDR_TUNER      0x60   

/*****************************************************************
** Function: _mt_fe_dmd_set_reg
**
**
** Description:	write data to demod register
**
**
** Inputs:
**
**	Parameter		Type		Description
**	----------------------------------------------------------
**	reg_index		U8			register index
**	data			U8			value to write
**
**
** Outputs:
**
**
*****************************************************************/
MT_FE_RET _mt_fe_dmd_set_reg(U8 reg_index, U8 data)
{
	

    MT_FE_RET ret = MtFeErr_Ok;

    if( mt_fe_set_one_register((uint8_t)DEV_ADDR, (uint8_t)reg_index, (uint8_t)data) != 0)
    {
        return MtFeErr_I2cErr;
    }

	return ret;
}



/*****************************************************************
** Function: _mt_fe_dmd_get_reg
**
**
** Description:	read data from demod register
**
**
** Inputs:
**
**	Parameter		Type		Description
**	----------------------------------------------------------
**	reg_index		U8			register index
**
**
**	Parameter		Type		Description
**	----------------------------------------------------------
**	p_buf			U8*			register data
**
**
*****************************************************************/
MT_FE_RET _mt_fe_dmd_get_reg(U8 reg_index, U8 *p_buf)
{
	
    MT_FE_RET ret = MtFeErr_Ok;

    if(mt_fe_get_one_register((uint8_t)DEV_ADDR, (uint8_t)reg_index, (uint8_t*)p_buf) != 0)
    {
        return MtFeErr_I2cErr;
    }

	return ret;
}





/*****************************************************************
** Function: _mt_fe_tn_set_reg
**
**
** Description:	write data to tuner register
**
**
** Inputs:
**
**	Parameter		Type		Description
**	----------------------------------------------------------
**	reg_index		U8			register index
**	data			U8			value to write
**
**
** Outputs:
**
**
*****************************************************************/
MT_FE_RET _mt_fe_tn_set_reg(U8 reg_index, U8 data)
{

	MT_FE_RET ret = MtFeErr_Ok;
	U8	val;

    val = 0x11;
	ret = _mt_fe_dmd_set_reg(0x03, val);
	if (ret != MtFeErr_Ok)
		return ret;
        
    
    if(mt_fe_set_one_register((uint8_t)DEV_ADDR_TUNER, (uint8_t)reg_index, (uint8_t)data) != 0)
    {
        return MtFeErr_I2cErr;
    }


	return ret;
}




/*****************************************************************
** Function: _mt_fe_tn_get_reg
**
**
** Description:	get tuner register data
**
**
** Inputs:
**
**	Parameter		Type		Description
**	----------------------------------------------------------
**	register		U8			register address
**
**
** Outputs:
**
**	Parameter		Type		Description
**	----------------------------------------------------------
**	p_buf			U8*			register data
**
**
*****************************************************************/
MT_FE_RET _mt_fe_tn_get_reg(U8 reg_index, U8 *p_buf)
{

    MT_FE_RET ret = MtFeErr_Ok;
    U8 val;

	/*open I2C repeater*/
	/*Do not care to close the I2C repeater, it will close by itself*/
	val = 0x12;
	ret = _mt_fe_dmd_set_reg(0x03, val);
	if (ret != MtFeErr_Ok)
		return ret;

    if(mt_fe_get_one_register((uint8_t)DEV_ADDR_TUNER, (uint8_t)reg_index, (uint8_t*)p_buf) != 0)
    {
        return MtFeErr_I2cErr;
    }

	return ret;
}




MT_FE_RET _mt_fe_tn_write(U8 *p_buf, U16 n_byte)
{
	MT_FE_RET ret = MtFeErr_Ok;
	U8	val;

	/*open I2C repeater*/
	/*Do not care to close the I2C repeater, it will close by itself*/
	val = 0x11;
	ret = _mt_fe_dmd_set_reg(0x03, val);
	if (ret != MtFeErr_Ok)
		return ret;

	return MtFeErr_Ok;
}




/*****************************************************************
** Function: _mt_fe_iic_write
**
**
** Description:	write n bytes via iic to device
**
**
** Inputs:
**
**
**	Parameter		Type		Description
**	----------------------------------------------------------
**	p_buf			U8*			pointer of the tuner data
**	n_byte			U16			the data length
**
**
** Outputs:		none
**
**
**
*****************************************************************/
MT_FE_RET _mt_fe_write_fw(U8 reg_index, U8 *p_buf, U16 n_byte)
{

    MT_FE_RET ret = MtFeErr_Ok;

    if(mt_fe_write_fw(DEV_ADDR, (uint8_t)reg_index, (uint8_t*)p_buf, (uint16_t)n_byte) != 0)
    {
        return MtFeErr_I2cErr;
    }

	return ret;
}
