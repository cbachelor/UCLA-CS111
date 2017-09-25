#!/usr/bin/python

"""Source code for CS111 Project 3b
   By Christopher Bachelor & Connor Borden					
"""

import sys
import csv
import math

""" Define classes for each type of data we want to store,
	essentially used like a struct to keep data we need organized
"""
class super_block:
	def __init__(self, totalBlocks, totalInodes, inodeSize, blockSize):
		self.totalBlocks = int(totalBlocks)
		self.totalInodes = int(totalInodes)
		self.inodeSize = int(inodeSize)
		self.blockSize = int(blockSize)

class group_block:
	def __init__(self, freeBlocks, freeInodes, blockBitmap, inodeBitmap, inodeTable):
		self.freeBlocks = int(freeBlocks)
		self.freeInodes = int(freeInodes)
		self.blockBitmap = int(blockBitmap)
		self.inodeBitmap = int(inodeBitmap)
		self.inodeTable = int(inodeTable)

class inode_block:
	def __init__(self, inodeNumber, fileType, mode, linkCount, numBlock):
		self.inodeNumber = int(inodeNumber)
		self.fileType = fileType
		self.mode = int(mode)
		self.linkCount = int(linkCount)
		self.numBlock = int(numBlock)
		self.calculatedLinkCount = 0

class dir_ent:
	def __init__(self, parentInode, byteOffset, inodeNumber, name):
		self.parentInode = int(parentInode)
		self.name = name
		self.byteOffset = int(byteOffset)
		self.inodeNumber = int(inodeNumber)

class indir_block:
	def __init__(self, ownerInode, level, blockOffset, indirectBlockNum, blockNum ):
		self.ownerInode = int(ownerInode)
		self.indirectBlockNum = int(indirectBlockNum)
		self.blockNum = int(blockNum)
		self.level = int(level)
		self.blockOffset = int(blockOffset)

class used_block_list:
	def __init__(self, inodeNumber, offset, level, printed):
		self.inodeNumber = int(inodeNumber)
		self.offset = int(offset)
		self.level = int(level)
		self.printed = printed #Boolean statement used to print the 1st occurence of a duplicate block, but when
							   #we don't still know that that block is a duplicate

#Print the Invalid Block, or Reserved Block statements for each block that appears in the csv
def invalid_block(blockNum, inode, offset, level):
	#if the logical number of the block is not within the total number of blocks specified in super block
	if blockNum > superBlock.totalBlocks or blockNum < 0:
		if level == 0:
			print "INVALID BLOCK " + blockNum + " IN INODE " + inode + " AT OFFSET " + offset
		if level == 1:
			print "INVALID INDIRECT BLOCK " + blockNum + " IN INODE " + inode + " AT OFFSET " + offset
		if level == 2:
			print "INVALID DOUBLE INDIRECT BLOCK " + blockNum + " IN INODE " + inode + " AT OFFSET " + offset
		if level == 3:
			print "INVALID TRIPPLE INDIRECT BLOCK " + blockNum + " IN INODE " + inode + " AT OFFSET " + offset
	#if the logical number of the block exists in the reserved block numbers
	if blockNum in reservedList:
		if level == 0:
			print "RESERVED BLOCK " + blockNum + " IN INODE " + inode + " AT OFFSET " + offset
		if level == 1:
			print "RESERVED INDIRECT BLOCK " + blockNum + " IN INODE " + inode + " AT OFFSET " + offset
		if level == 2:
			print "RESERVED DOUBLE INDIRECT BLOCK " + blockNum + " IN INODE " + inode + " AT OFFSET " + offset
		if level == 3:
			print "RESERVED TRIPPLE INDIRECT BLOCK " + blockNum + " IN INODE " + inode + " AT OFFSET " + offset	

#Handles printing statements when there are duplicates
def print_duplicate(blockNum, parentInode, offset, level):
	#if the 1st appearance of a duplicate block hasn't been printed yet, print it.
	if usedBlockList[blockNum].printed == false:
		if usedBlockList[blockNum].level == 0:
			print "DUPLICATE BLOCK " + blockNum + " IN INODE " + usedBlockList[blockNum].inodeNumber + " AT OFFSET " + usedBlockList[blockNum].offset
		if usedBlockList[blockNum].level == 1:
			print "DUPLICATE INDIRECT BLOCK " + blockNum + " IN INODE " + usedBlockList[blockNum].inodeNumber + " AT OFFSET " + usedBlockList[blockNum].offset
		if usedBlockList[blockNum].level == 2:
			print "DUPLICATE DOUBLE INDIRECT BLOCK " + blockNum + " IN INODE " + usedBlockList[blockNum].inodeNumber + " AT OFFSET " + usedBlockList[blockNum].offset
		if usedBlockList[blockNum].level == 3:
			print "DUPLICATE TRIPPLE INDIRECT BLOCK " + blockNum + " IN INODE " + usedBlockList[blockNum].inodeNumber + " AT OFFSET " + usedBlockList[blockNum].offset
		#Once it's been printed, make sure it's not printed again
		usedBlockList[blockNum].printed == false
	#Genereal case: print the duplicated block as it appears
	if level == 0:
		print "DUPLICATE BLOCK " + blockNum + " IN INODE " + inode + " AT OFFSET " + offset
	if level == 1:
		print "DUPLICATE INDIRECT BLOCK " + blockNum + " IN INODE " + inode + " AT OFFSET " + offset
	if level == 2:
		print "DUPLICATE DOUBLE INDIRECT BLOCK " + blockNum + " IN INODE " + inode + " AT OFFSET " + offset
	if level == 3:
		print "DUPLICATE TRIPPLE INDIRECT BLOCK " + blockNum + " IN INODE " + inode + " AT OFFSET " + offset


def parse_indirect_list(indirectList):
	for element in indirectList:
		for entry in indirectList[element]:
			invalid_block(entry.blockNum, entry.ownerInode, entry.byteOffset, entry.level - 1)
			if entry.blockNum in usedBlockList:
				print_duplicate(entry.blockNum, entry.parentInode, entry.blockOffset, entry.level-1)
			else
				usedBlockList[entry.blockNum] = used_block_list(entry.parentInode, entry.byteOffset, entry.level - 1, false)
			
def main():
	f = open(sys.argv[1], 'r')
	global inodeList
	global iFreeList
	global bFreeList
	global dirEntList
	global reservedList
	global usedBlockList
	usedBlockList = {}
	reservedList = [0,1,2]
	inodeList = {}
	dirEntList = {}
	indirectList = {}
	reader = csv.reader(f)
	for row in reader:
		if row[0] == "SUPERBLOCK":
			global superBlock 
			superBlock = super_block(row[1], row[2], row[4], row[3])
			iFreeList = [1] * superBlock.totalInodes
			bFreeList = [1] * superBlock.totalBlocks
		if row[0] == "GROUP":
			global groupBlock
			groupBlock = group_block(row[4], row[5], row[6], row[7], row[8])
			reservedList.append(groupBlock.blockBitmap)
			reservedList.append(groupBlock.inodeBitmap)
			k = math.ceil(superBlock.inodeSize * superBlock.totalInodes / superBlock.blockSize)
			for i in range(1,k):
				reservedList.append(groupBlock.inodeTable + i - 1)
				usedBlockList[groupBlock.inodeTable + i -1] = used_block_list(-1, -1, -1, false)
			usedBlockList[groupBlock.blockBitmap] = used_block_list(-1, -1, -1, false)
			usedBlockList[groupBlock.inodeBitmap] = used_block_list(-1, -1, -1, false)
		if row[0] == "INODE":
			inode = {}
			inode[row[1]]  = inode_block(row[1], row[2], row[3], row[6], row[11])
			if iFreeList[int(row[1]) - 1] == 0:
				print "unallocated inode in use"	#TODO
			if inode[row[1]] in inodeList:
				print "double inodeList"		#TODO
			else:
				inodeList.update(inode)
				print inodeList[row[1]].inodeNumber
			for i in range(0,11):
				offset = i
				invalid_block(row[i + 12], row[1], offset, 0)
				if row[i + 12] in usedBlockList:
					print_duplicate(row[i + 12], row[1], i, 0)
				else
					usedBlockList[row[i + 12]] = used_block_list(row[1], i, 0, false)
			if row[24] != 0:
				invalid_block(row[24], row[1], 12, 1)
				if row[i + 12] in usedBlockList:
					print_duplicate(row[24], row[1], 12, 1)
				else
					usedBlockList[row[i + 12]] = used_block_list(row[1], 12, 1, false)
			if row[25] != 0:
				invalid_block(row[25], row[1], 12 + superBlock.blockSize/4, 2)
				if row[i + 12] in usedBlockList:		
					print_duplicate(row[25], row[1], 12 + superBlock.blockSize/4, 2)
				else
					usedBlockList[row[i + 12]] = used_block_list(row[1], 12 + superBlock.blockSize/4, 2, false)
			if row[26] != 0:
				invalid_block(row[26], row[1], 12 + superBlock.blockSize/4 * superBlock.blockSize/4, 3)
				if row[i + 12] in usedBlockList:
					print_duplicate(row[26], row[1], 12 + superBlock.blockSize/4 * superBlock.blockSize/4, 3)
				else
					usedBlockList[row[i + 12]] = used_block_list(row[1], 12 + superBlock.blockSize/4 * superBlock.blockSize/4, 3, false)
			
		if row[0] == "IFREE":
			iFreeList[int(row[1]) - 1] = 0
		if row[0] == "BFREE":
			bFreeList[int(row[1]) - 1] = 0
		if row[0] == "DIRENT":
			if row[1] in dirEntList:
				dirEntList[row[1]].append(dir_ent(row[1], row[2], row[3], row[6]))
			else:
				dirEntList[row[1]] = [dir_ent(row[1], row[2], row[3], row[6])]
		if row[0] == "INDIRECT":
			if row[1] in indirectList:
				indirectList[row[1]].append(indir_block(row[1],row[2], row[3],row[4], row[5]))
			else:
				indirectList[row[1]] = [indir_block(row[1],row[2], row[3],row[4], row[5])]
		

		parse_indirect_list(indirectList)
		for i in range(0, superBlock.totalBlocks - 1):
			if bFreeList[i] == 1:
				if (i + 1) not in usedBlockList:
					print "UNREFERENCED BLOCK " + (i + 1)
			else
				if (i + 1) in usedBlockList:
					print "ALLOCATED BLOCK " + (i + 1) + " ON FREELIST"


		for i in range(0, superBlock.totalInodes - 1):
			if iFreeList[i] == 1:
				if (i + 1) not in inodeList:
					print "UNALLOCATED INODE " + (i + 1) + " NOT ON FREELIST"
			else
				if (i + 1) in inodeList:
					print "ALLOCATED INODE " + (i + 1) + " ON FREELIST"

		for key in dirEntList:
			for element in dirEntList[key]:
				if element.inodeNumber in inodesList:
					inodesList[element.inodeNumber].calculatedLinkCount += 1
				if element.inodeNumber > superBlock.totalInodes or element.inodeNumber < 0:
					print "DIRECTORY INODE " + key + " NAME '" + element.name + "' INVALID INODE " + element.inodeNumber
				if iFreeList[element.inodeNumber] == 0:
					print "DIRECTORY INODE " + key + " NAME '" + element.name + "' UNALLOCATED INODE " + element.inodeNumber
				if element.name == ".":
					if element.parentInode != element.inodeNumber:
						print "DIRECTORY INODE " + key + " NAME '" + element.name "' LINK TO INODE " + element.inodeNumber + " SHOULD BE " + key;
				if element.name == "..":
					if element.inodeNumber in dirEntList:
						found = false
						for entry in dirEntList[element.inodeNumber]:
							if dirEntList[element.inodeNumber][entry].inodeNumber == key:
								found = true
						if found == false:
							if key == 2:
								print "DIRECTORY INODE 2 NAME '..' LINK TO INODE " + element.inodeNumber + " SHOULD BE 2"
							else:
								for i in dirEntList:
									for j in dirEntList[i]:
										if j.inodeNumber == key and j.name != "." and j.name != "..":
											print "DIRECTORY INODE " + key + " NAME '..' LINK TO INODE " + element.inodeNumber + " SHOULD BE " + j.inodeNumber
							

		for inode in inodeList:
			if inodeList[inode].linkCount != inodeList[inode].calculatedLinkCount:
				print "INODE " + inode + " HAS " + inodeList[inode].calculatedLinkCount + " LINKS BUT LINKCOUNT IS " inodeList[inode].linkCount
	





if __name__ == "__main__":
	main()
