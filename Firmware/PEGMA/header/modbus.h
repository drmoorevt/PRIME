#ifndef MODBUS_H
#define MODBUS_H

#include "types.h"

void ModbusMaster_SendRequest(void);

void ModbusMaster_ReadDiscreteInputs(void);
void ModbusMaster_ReadCoils(void);
void ModbusMaster_ReadInputRegister(void);
void ModbusMaster_ReadHoldingRegister(void);
void ModbusMaster_WriteHoldingRegister(void);
void ModbusMaster_ReadMultipleRegisters(void);
void ModbusMaster_WriteMultipleRegisters(void);

void ModbusMaster_ProcessResponse(void);

void Modbus_SlaveReadDiscreteInputs(void);
void Modbus_SlaveReadCoils(void);
void Modbus_SlaveReadInputRegister(void);
void Modbus_SlaveReadHoldingRegister(void);
void Modbus_SlaveWriteHoldingRegister(void);
void Modbus_SlaveReadMultipleRegisters(void);
void Modbus_SlaveWriteMultipleRegisters(void);

void Modbus_SlaveProcessRequest(void);


#endif // MODBUS_H
